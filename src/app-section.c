/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2009 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <libindicator/indicator-desktop-shortcuts.h>
#include "app-section.h"
#include "dbus-data.h"

typedef struct _AppSectionPrivate AppSectionPrivate;

struct _AppSectionPrivate
{
	GDesktopAppInfo * appinfo;
	guint unreadcount;

	IndicatorDesktopShortcuts * ids;

	GMenu *menu;
	GSimpleActionGroup *static_shortcuts;
};

#define APP_SECTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), APP_SECTION_TYPE, AppSectionPrivate))

enum {
	PROP_0,
	PROP_APPINFO,
	NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

/* Prototypes */
static void app_section_class_init   (AppSectionClass *klass);
static void app_section_init         (AppSection *self);
static void app_section_get_property (GObject    *object,
				      guint       property_id,
				      GValue     *value,
				      GParamSpec *pspec);
static void app_section_set_property (GObject      *object,
				      guint         property_id,
				      const GValue *value,
				      GParamSpec   *pspec);
static void app_section_dispose      (GObject *object);
static void activate_cb              (GSimpleAction *action,
				      GVariant *param,
				      gpointer userdata);
static void app_section_set_app_info (AppSection *self,
				      GDesktopAppInfo *appinfo);

/* GObject Boilerplate */
G_DEFINE_TYPE (AppSection, app_section, G_TYPE_OBJECT);

static void
app_section_class_init (AppSectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AppSectionPrivate));

	object_class->get_property = app_section_get_property;
	object_class->set_property = app_section_set_property;
	object_class->dispose = app_section_dispose;

	properties[PROP_APPINFO] = g_param_spec_object ("app-info",
							"AppInfo",
							"The GAppInfo for the app that this menu represents",
							G_TYPE_APP_INFO,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
app_section_init (AppSection *self)
{
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);

	priv->appinfo = NULL;
	priv->unreadcount = 0;

	priv->menu = g_menu_new ();
	priv->static_shortcuts = g_simple_action_group_new ();

	return;
}

static void
app_section_get_property (GObject    *object,
			  guint       property_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	AppSection *self = APP_SECTION (object);

	switch (property_id)
	{
	case PROP_APPINFO:
		g_value_set_object (value, app_section_get_app_info (self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
app_section_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	AppSection *self = APP_SECTION (object);

	switch (property_id)
	{
	case PROP_APPINFO:
		app_section_set_app_info (self, g_value_get_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}
static void
app_section_dispose (GObject *object)
{
	AppSection * self = APP_SECTION(object);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);

	g_clear_object (&priv->menu);
	g_clear_object (&priv->static_shortcuts);

	if (priv->ids != NULL) {
		g_object_unref(priv->ids);
		priv->ids = NULL;
	}

	if (priv->appinfo != NULL) {
		g_object_unref(priv->appinfo);
		priv->appinfo = NULL;
	}

	G_OBJECT_CLASS (app_section_parent_class)->dispose (object);
}

/* Respond to one of the shortcuts getting clicked on. */
static void
nick_activate_cb (GSimpleAction *action,
		  GVariant *param,
		  gpointer userdata)
{
	const gchar * nick = g_action_get_name (G_ACTION (action));
	AppSection * mi = APP_SECTION (userdata);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(mi);

	g_return_if_fail(priv->ids != NULL);

	if (!indicator_desktop_shortcuts_nick_exec(priv->ids, nick)) {
		g_warning("Unable to execute nick '%s' for desktop file '%s'",
			  nick, g_desktop_app_info_get_filename (priv->appinfo));
	}
}

static void
app_section_set_app_info (AppSection *self,
			  GDesktopAppInfo *appinfo)
{
	AppSectionPrivate *priv = APP_SECTION_GET_PRIVATE (self);
	GSimpleAction *launch;
	gchar *label;

	g_return_if_fail (priv->appinfo == NULL);

	if (appinfo == NULL) {
		g_warning ("appinfo must not be NULL");
		return;
	}

	priv->appinfo = g_object_ref (appinfo);

	launch = g_simple_action_new ("launch", NULL);
	g_signal_connect (launch, "activate", G_CALLBACK (activate_cb), self);
	g_simple_action_group_insert (priv->static_shortcuts, G_ACTION (launch));

	if (priv->unreadcount > 0)
		label = g_strdup_printf("%s (%d)", app_section_get_name (self), priv->unreadcount);
	else
		label = g_strdup(app_section_get_name (self));

	/* Start to build static shortcuts */
	priv->ids = indicator_desktop_shortcuts_new(g_desktop_app_info_get_filename (priv->appinfo), "Messaging Menu");
	const gchar ** nicks = indicator_desktop_shortcuts_get_nicks(priv->ids);
	gint i;
	for (i = 0; nicks[i] != NULL; i++) {
		gchar *name;
		GSimpleAction *action;
		GMenuItem *item;

		name = indicator_desktop_shortcuts_nick_get_name(priv->ids, nicks[i]);

		action = g_simple_action_new (name, NULL);
		g_signal_connect(action, "activate", G_CALLBACK (nick_activate_cb), self);
		g_simple_action_group_insert (priv->static_shortcuts, G_ACTION (action));

		item = g_menu_item_new (name, name);
		g_menu_append_item (priv->menu, item);

		g_object_unref (item);
		g_free(name);
	}

	g_free(label);
	g_object_unref (launch);
}

AppSection *
app_section_new (GDesktopAppInfo *appinfo)
{
	return g_object_new (APP_SECTION_TYPE,
			     "app-info", appinfo,
			     NULL);
}

static void
activate_cb (GSimpleAction *action,
	     GVariant *param,
	     gpointer userdata)
{
	AppSection * mi = APP_SECTION (userdata);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(mi);
	GError *error = NULL;

	if (!g_app_info_launch (G_APP_INFO (priv->appinfo), NULL, NULL, &error)) {
		g_warning("Unable to execute application for desktop file '%s'",
			  g_desktop_app_info_get_filename (priv->appinfo));
	}
}

guint
app_section_get_count (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), 0);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);

	return priv->unreadcount;
}

const gchar *
app_section_get_name (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), NULL);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);

	if (priv->appinfo) {
		return g_app_info_get_name(G_APP_INFO(priv->appinfo));
	}
	return NULL;
}

const gchar *
app_section_get_desktop (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), NULL);
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);
	if (priv->appinfo)
		return g_desktop_app_info_get_filename (priv->appinfo);
	else
		return NULL;
}

GMenuModel *
app_section_get_menu (AppSection *self)
{
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);
	return G_MENU_MODEL (priv->menu);
}

GAppInfo *
app_section_get_app_info (AppSection *self)
{
	AppSectionPrivate * priv = APP_SECTION_GET_PRIVATE(self);
	return G_APP_INFO (priv->appinfo);
}

GMenuItem *
app_section_create_menu_item (AppSection *self)
{
	AppSectionPrivate *priv = APP_SECTION_GET_PRIVATE (self);
	GMenuItem *item;
	const gchar *name;
	gchar *iconstr;

	g_return_val_if_fail (priv->appinfo != NULL, NULL);

	name = g_app_info_get_name (G_APP_INFO (priv->appinfo));
	iconstr = g_icon_to_string (g_app_info_get_icon (G_APP_INFO (priv->appinfo)));

	item = g_menu_item_new (name, "launch");
	g_menu_item_set_attribute (item, INDICATOR_MENU_ATTRIBUTE_ICON_NAME, "s", iconstr);
	g_menu_item_set_section (item, G_MENU_MODEL (priv->menu));

	g_free(iconstr);
	return item;
}

