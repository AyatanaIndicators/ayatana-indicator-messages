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

#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>
#include <libindicator/indicator-desktop-shortcuts.h>
#include "launcher-menu-item.h"
#include "dbus-data.h"
#include "default-applications.h"
#include "seen-db.h"

enum {
	NAME_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _LauncherMenuItemPrivate LauncherMenuItemPrivate;
struct _LauncherMenuItemPrivate
{
	GAppInfo * appinfo;
	gchar * desktop;
	IndicatorDesktopShortcuts * ids;
	GList * shortcuts;
};

#define LAUNCHER_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LAUNCHER_MENU_ITEM_TYPE, LauncherMenuItemPrivate))

#define NICK_DATA  "ids-nick-data"

/* Prototypes */
static void launcher_menu_item_class_init (LauncherMenuItemClass *klass);
static void launcher_menu_item_init       (LauncherMenuItem *self);
static void launcher_menu_item_dispose    (GObject *object);
static void launcher_menu_item_finalize   (GObject *object);
static void activate_cb (LauncherMenuItem * self, guint timestamp, gpointer data);
static void nick_activate_cb (LauncherMenuItem * self, guint timestamp, gpointer data);


G_DEFINE_TYPE (LauncherMenuItem, launcher_menu_item, DBUSMENU_TYPE_MENUITEM);

static void
launcher_menu_item_class_init (LauncherMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LauncherMenuItemPrivate));

	object_class->dispose = launcher_menu_item_dispose;
	object_class->finalize = launcher_menu_item_finalize;

	signals[NAME_CHANGED] =  g_signal_new(LAUNCHER_MENU_ITEM_SIGNAL_NAME_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (LauncherMenuItemClass, name_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__STRING,
	                                      G_TYPE_NONE, 1, G_TYPE_STRING);

	return;
}

static void
launcher_menu_item_init (LauncherMenuItem *self)
{
	g_debug("Building new Launcher Menu Item");
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	priv->appinfo = NULL;
	priv->desktop = NULL;

	priv->ids = NULL;
	priv->shortcuts = NULL;

	return;
}

static void
func_unref (gpointer data, gpointer user_data)
{
	g_object_unref(G_OBJECT(data));
	return;
}

static void
launcher_menu_item_dispose (GObject *object)
{
	LauncherMenuItem * self = LAUNCHER_MENU_ITEM(object);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	if (priv->appinfo != NULL) {
		g_object_unref(priv->appinfo);
		priv->appinfo = NULL;
	}

	if (priv->ids != NULL) {
		g_object_unref(priv->ids);
		priv->ids = NULL;
	}

	if (priv->shortcuts != NULL) {
		g_list_foreach(priv->shortcuts, func_unref, NULL);
		g_list_free(priv->shortcuts);
		priv->shortcuts = NULL;
	}

	G_OBJECT_CLASS (launcher_menu_item_parent_class)->dispose (object);
}

static void
launcher_menu_item_finalize (GObject *object)
{
	LauncherMenuItem * self = LAUNCHER_MENU_ITEM(object);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	if (priv->desktop != NULL) {
		g_free(priv->desktop);
		priv->desktop = NULL;
	}

	G_OBJECT_CLASS (launcher_menu_item_parent_class)->finalize (object);

	return;
}

LauncherMenuItem *
launcher_menu_item_new (const gchar * desktop_file)
{
	LauncherMenuItem * self = g_object_new(LAUNCHER_MENU_ITEM_TYPE, NULL);
	g_debug("\tDesktop file: %s", desktop_file);

	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	/* Parse the desktop file we've been given. */
	priv->appinfo = G_APP_INFO(g_desktop_app_info_new_from_filename(desktop_file));
	priv->desktop = g_strdup(desktop_file);

	/* Set the appropriate values on this menu item based on the
	   app info that we've parsed */
	g_debug("\tName: %s", launcher_menu_item_get_name(self));

	const gchar * default_name = NULL;
	
	if (seen_db_seen(desktop_file)) {
		default_name = get_default_name(desktop_file);
	} else {
		default_name = get_default_setup(desktop_file);
	}

	if (default_name == NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_LABEL, launcher_menu_item_get_name(self));
	} else {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_LABEL, _(default_name));
	}

	gchar * iconstr;
	if (default_name == NULL) {
		iconstr = launcher_menu_item_get_icon(self);
	} else {
		iconstr = g_strdup(get_default_icon(desktop_file));
	}
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_ICON_NAME, iconstr);
	g_free(iconstr);
	dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);

	g_signal_connect(G_OBJECT(self), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(activate_cb), NULL);

	/* Start to build static shortcuts */
	if (seen_db_seen(desktop_file)) {
		priv->ids = indicator_desktop_shortcuts_new(priv->desktop, "Messaging Menu");
		const gchar ** nicks = indicator_desktop_shortcuts_get_nicks(priv->ids);
		gint i;
		for (i = 0; nicks[i] != NULL; i++) {
			DbusmenuMenuitem * mi = dbusmenu_menuitem_new();
			dbusmenu_menuitem_property_set(mi, DBUSMENU_MENUITEM_PROP_TYPE, APPLICATION_MENUITEM_TYPE);
			g_object_set_data(G_OBJECT(mi), NICK_DATA, (gpointer)nicks[i]);

			dbusmenu_menuitem_property_set(mi, DBUSMENU_MENUITEM_PROP_LABEL, indicator_desktop_shortcuts_nick_get_name(priv->ids, nicks[i]));
			g_signal_connect(G_OBJECT(mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(nick_activate_cb), self);

			priv->shortcuts = g_list_append(priv->shortcuts, mi);
		}
	}

	/* Check to see if we should be eclipsed */
	if (priv->appinfo == NULL) {
		launcher_menu_item_set_eclipsed(self, TRUE);
	}

	return self;
}

const gchar *
launcher_menu_item_get_name (LauncherMenuItem * appitem)
{
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(appitem);

	if (priv->appinfo == NULL) {
		return NULL;
	} else {
		return g_app_info_get_name(priv->appinfo);
	}
}

/* Respond to one of the shortcuts getting clicked on. */
static void
nick_activate_cb (LauncherMenuItem * self, guint timestamp, gpointer data)
{
	gchar * nick = (gchar *)g_object_get_data(G_OBJECT(self), NICK_DATA);
	LauncherMenuItem * lmi = LAUNCHER_MENU_ITEM(data);

	g_return_if_fail(nick != NULL);
	g_return_if_fail(lmi != NULL);

	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(lmi);
	
	g_return_if_fail(priv->ids != NULL);

	if (!indicator_desktop_shortcuts_nick_exec(priv->ids, nick)) {
		g_warning("Unable to execute nick '%s' for desktop file '%s'", nick, priv->desktop);
	}

	return;
}

gchar *
launcher_menu_item_get_icon (LauncherMenuItem * appitem)
{
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(appitem);

	if (priv->appinfo == NULL) {
		return NULL;
	} else {
		GIcon * icon = g_app_info_get_icon(priv->appinfo);
		gchar * iconstr = g_icon_to_string(icon);
		return iconstr;
	}
}

/* When the menu item is clicked on it tries to launch
   the application that is represented by the desktop file */
static void
activate_cb (LauncherMenuItem * self, guint timestamp, gpointer data)
{
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);
	g_return_if_fail(priv->appinfo != NULL);

	GError * error = NULL;
	if (!g_app_info_launch(priv->appinfo, NULL, NULL, &error)) {
		g_warning("Application failed to launch '%s' because: %s", launcher_menu_item_get_name(self), error->message);
		g_error_free(error);
	}

	return;
}

const gchar *
launcher_menu_item_get_desktop (LauncherMenuItem * launchitem)
{
	g_return_val_if_fail(IS_LAUNCHER_MENU_ITEM(launchitem), NULL);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(launchitem);
	return priv->desktop;
}

/* Gets the decription for the item that should
   go in the messaging menu */
const gchar *
launcher_menu_item_get_description (LauncherMenuItem * li)
{
	g_return_val_if_fail(IS_LAUNCHER_MENU_ITEM(li), NULL);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(li);
	return g_app_info_get_description(priv->appinfo);
}

/* Apply the eclipse value to all the shortcuts */
static void
eclipse_shortcuts_cb (gpointer data, gpointer user_data)
{
	DbusmenuMenuitem * mi = DBUSMENU_MENUITEM(data);
	g_return_if_fail(mi != NULL);

	gboolean eclipsed = GPOINTER_TO_UINT(user_data);
	
	dbusmenu_menuitem_property_set_bool(mi, DBUSMENU_MENUITEM_PROP_VISIBLE, !eclipsed);
	return;
}

/* Hides the menu item based on whether it is eclipsed
   or not. */
void
launcher_menu_item_set_eclipsed (LauncherMenuItem * li, gboolean eclipsed)
{
	g_return_if_fail(IS_LAUNCHER_MENU_ITEM(li));
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(li);

	g_debug("Laucher '%s' is %s", launcher_menu_item_get_name(li), eclipsed ? "now eclipsed" : "shown again");
	dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(li), DBUSMENU_MENUITEM_PROP_VISIBLE, !eclipsed);

	g_list_foreach(priv->shortcuts, eclipse_shortcuts_cb, GINT_TO_POINTER(eclipsed));
	
	/* If we're being reshown let's re-evaluate how we should be
	   showing the label */
	if (!eclipsed) {
		const gchar * default_name = NULL;
		
		if (seen_db_seen(priv->desktop)) {
			default_name = get_default_name(priv->desktop);
		} else {
			default_name = get_default_setup(priv->desktop);
		}

		if (default_name == NULL) {
			dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(li), DBUSMENU_MENUITEM_PROP_LABEL, launcher_menu_item_get_name(li));
		} else {
			dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(li), DBUSMENU_MENUITEM_PROP_LABEL, _(default_name));
		}
	}

	return;
}

/* Check to see if this item is eclipsed */
gboolean
launcher_menu_item_get_eclipsed (LauncherMenuItem * li)
{
	gboolean show = dbusmenu_menuitem_property_get_bool(DBUSMENU_MENUITEM(li), DBUSMENU_MENUITEM_PROP_VISIBLE);
	g_debug("Launcher check eclipse: %s", show ? "false" : "true");
	return !show;
}

/* Gets the shortcuts that are associated with this
   launcher.  They're a list of DbusmenuMenuitems */
GList *
launcher_menu_item_get_items (LauncherMenuItem * li)
{
	g_return_val_if_fail(IS_LAUNCHER_MENU_ITEM(li), NULL);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(li);
	return priv->shortcuts;
}
