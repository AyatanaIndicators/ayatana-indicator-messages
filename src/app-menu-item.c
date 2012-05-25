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
#include "app-menu-item.h"
#include "dbus-data.h"

typedef struct _AppMenuItemPrivate AppMenuItemPrivate;

struct _AppMenuItemPrivate
{
	GDesktopAppInfo * appinfo;
	guint unreadcount;

	IndicatorDesktopShortcuts * ids;

	GMenu *menu;
	GSimpleActionGroup *static_shortcuts;
};

#define APP_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), APP_MENU_ITEM_TYPE, AppMenuItemPrivate))

/* Prototypes */
static void app_menu_item_class_init (AppMenuItemClass *klass);
static void app_menu_item_init       (AppMenuItem *self);
static void app_menu_item_dispose    (GObject *object);
static void activate_cb (GSimpleAction *action,
			 GVariant *param,
			 gpointer userdata);

/* GObject Boilerplate */
G_DEFINE_TYPE (AppMenuItem, app_menu_item, G_TYPE_OBJECT);

static void
app_menu_item_class_init (AppMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AppMenuItemPrivate));

	object_class->dispose = app_menu_item_dispose;
}

static void
app_menu_item_init (AppMenuItem *self)
{
	g_debug("Building new App Menu Item");
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(self);

	priv->appinfo = NULL;
	priv->unreadcount = 0;

	priv->menu = g_menu_new ();
	priv->static_shortcuts = g_simple_action_group_new ();

	return;
}

static void
app_menu_item_dispose (GObject *object)
{
	AppMenuItem * self = APP_MENU_ITEM(object);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(self);

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

	G_OBJECT_CLASS (app_menu_item_parent_class)->dispose (object);
}

/* Respond to one of the shortcuts getting clicked on. */
static void
nick_activate_cb (GSimpleAction *action,
		  GVariant *param,
		  gpointer userdata)
{
	const gchar * nick = g_action_get_name (G_ACTION (action));
	AppMenuItem * mi = APP_MENU_ITEM (userdata);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(mi);

	g_return_if_fail(priv->ids != NULL);

	if (!indicator_desktop_shortcuts_nick_exec(priv->ids, nick)) {
		g_warning("Unable to execute nick '%s' for desktop file '%s'",
			  nick, g_desktop_app_info_get_filename (priv->appinfo));
	}
}

static void
app_menu_item_set_appinfo (AppMenuItem *self,
			   GDesktopAppInfo *appinfo)
{
	AppMenuItemPrivate *priv = APP_MENU_ITEM_GET_PRIVATE (self);
	GSimpleAction *launch;
	GMenuItem *menuitem;
	GIcon *icon;
	gchar *iconstr = NULL;
	gchar *label;

	g_return_if_fail (appinfo != NULL);

	g_clear_object (&priv->appinfo);
	priv->appinfo = g_object_ref (appinfo);

	icon = g_app_info_get_icon (G_APP_INFO(priv->appinfo));
	iconstr = g_icon_to_string (icon);

	launch = g_simple_action_new ("launch", NULL);
	g_signal_connect (launch, "activate", G_CALLBACK (activate_cb), self);
	g_simple_action_group_insert (priv->static_shortcuts, G_ACTION (launch));

	if (priv->unreadcount > 0)
		label = g_strdup_printf("%s (%d)", app_menu_item_get_name (self), priv->unreadcount);
	else
		label = g_strdup(app_menu_item_get_name (self));

	menuitem = g_menu_item_new (label, "launch");
	g_menu_item_set_attribute (menuitem, INDICATOR_MENU_ATTRIBUTE_ICON_NAME, "s", iconstr);
	g_menu_append_item (priv->menu, menuitem);

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
	g_free(iconstr);
	g_object_unref (launch);
	g_object_unref (menuitem);
}

AppMenuItem *
app_menu_item_new (GDesktopAppInfo *appinfo)
{
	AppMenuItem *self = g_object_new(APP_MENU_ITEM_TYPE, NULL);
	if (appinfo)
		app_menu_item_set_appinfo (self, appinfo);
	return self;
}

static void
activate_cb (GSimpleAction *action,
	     GVariant *param,
	     gpointer userdata)
{
	AppMenuItem * mi = APP_MENU_ITEM (userdata);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(mi);
	GError *error = NULL;

	if (!g_app_info_launch (G_APP_INFO (priv->appinfo), NULL, NULL, &error)) {
		g_warning("Unable to execute application for desktop file '%s'",
			  g_desktop_app_info_get_filename (priv->appinfo));
	}
}

guint
app_menu_item_get_count (AppMenuItem * appitem)
{
	g_return_val_if_fail(IS_APP_MENU_ITEM(appitem), 0);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(appitem);

	return priv->unreadcount;
}

const gchar *
app_menu_item_get_name (AppMenuItem * appitem)
{
	g_return_val_if_fail(IS_APP_MENU_ITEM(appitem), NULL);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(appitem);

	if (priv->appinfo) {
		return g_app_info_get_name(G_APP_INFO(priv->appinfo));
	}
	return NULL;
}

const gchar *
app_menu_item_get_desktop (AppMenuItem * appitem)
{
	g_return_val_if_fail(IS_APP_MENU_ITEM(appitem), NULL);
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(appitem);
	if (priv->appinfo)
		return g_desktop_app_info_get_filename (priv->appinfo);
	else
		return NULL;
}

GMenuModel *
app_menu_item_get_menu (AppMenuItem *appitem)
{
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(appitem);
	return G_MENU_MODEL (priv->menu);
}

