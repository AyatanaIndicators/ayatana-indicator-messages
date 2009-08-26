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
#include "launcher-menu-item.h"

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
};

#define LAUNCHER_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LAUNCHER_MENU_ITEM_TYPE, LauncherMenuItemPrivate))

/* Prototypes */
static void launcher_menu_item_class_init (LauncherMenuItemClass *klass);
static void launcher_menu_item_init       (LauncherMenuItem *self);
static void launcher_menu_item_dispose    (GObject *object);
static void launcher_menu_item_finalize   (GObject *object);
static void activate_cb (LauncherMenuItem * self, gpointer data);


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

	return;
}

static void
launcher_menu_item_dispose (GObject *object)
{
	// LauncherMenuItem * self = LAUNCHER_MENU_ITEM(object);
	// LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	G_OBJECT_CLASS (launcher_menu_item_parent_class)->dispose (object);
}

static void
launcher_menu_item_finalize (GObject *object)
{
	LauncherMenuItem * self = LAUNCHER_MENU_ITEM(object);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);

	if (priv->appinfo != NULL) {
		g_object_unref(priv->appinfo);
		priv->appinfo = NULL;
	}

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

	priv->appinfo = G_APP_INFO(g_desktop_app_info_new_from_filename(desktop_file));
	priv->desktop = g_strdup(desktop_file);

	g_debug("\tName: %s", launcher_menu_item_get_name(self));
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), "label", launcher_menu_item_get_name(self));

	g_signal_connect(G_OBJECT(self), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(activate_cb), NULL);

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

/* When the menu item is clicked on it tries to launch
   the application that is represented by the desktop file */
static void
activate_cb (LauncherMenuItem * self, gpointer data)
{
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(self);
	g_return_if_fail(priv->appinfo != NULL);

	/* This should manage the X stuff for us */
	GdkAppLaunchContext * context = gdk_app_launch_context_new();

	/* Using the current time as we don't have the event
	   time as that's not sent across the bus */
	GTimeVal time;
	g_get_current_time(&time);
	gdk_app_launch_context_set_timestamp(context, time.tv_usec / 1000);

	GError * error = NULL;
	if (!g_app_info_launch(priv->appinfo, NULL, G_APP_LAUNCH_CONTEXT(context), &error)) {
		g_warning("Application failed to launch '%s' because: %s", launcher_menu_item_get_name(self), error->message);
		g_error_free(error);
	}

	g_object_unref(G_OBJECT(context));

	return;
}

const gchar *
launcher_menu_item_get_desktop (LauncherMenuItem * launchitem)
{
	g_return_val_if_fail(IS_LAUNCHER_MENU_ITEM(launchitem), NULL);
	LauncherMenuItemPrivate * priv = LAUNCHER_MENU_ITEM_GET_PRIVATE(launchitem);
	return priv->desktop;
}

/* Hides the menu item based on whether it is eclipsed
   or not. */
void
launcher_menu_item_set_eclipsed (LauncherMenuItem * li, gboolean eclipsed)
{
	g_debug("Laucher '%s' is %s", launcher_menu_item_get_name(li), eclipsed ? "now eclipsed" : "shown again");
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(li), "show", eclipsed ? "false" : "true");
	return;
}

gboolean
launcher_menu_item_get_eclipsed (LauncherMenuItem * li)
{
	const gchar * show = dbusmenu_menuitem_property_get(DBUSMENU_MENUITEM(li), "show");
	if (show == NULL) {
		return FALSE;
	}
	if (!g_strcmp0(show, "true")) {
		return TRUE;
	}
	return FALSE;
}
