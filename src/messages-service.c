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

#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <config.h>
#include <pango/pango-utils.h>
#include <libindicator/indicator-service.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include "app-menu-item.h"
#include "dbus-data.h"
#include "messages-service-dbus.h"
#include "status-items.h"

static IndicatorService * service = NULL;
static GHashTable *applications;

static GDBusConnection *bus;
static GSimpleActionGroup *actions;
static GMenu *menu;
static GSettings *settings;
static GMainLoop * mainloop = NULL;

static MessageServiceDbus * dbus_interface = NULL;


static void
add_application (const gchar *desktop_id,
		 const gchar *menu_path)
{
	GDesktopAppInfo *appinfo;
	const gchar *desktop_file;

	appinfo = g_desktop_app_info_new (desktop_id);
	if (!appinfo) {
		g_warning ("could not find a desktop file with id '%s'\n", desktop_id);
		return;
	}

	desktop_file = g_desktop_app_info_get_filename (appinfo);

	if (!g_hash_table_lookup (applications, desktop_file)) {
		AppMenuItem *menuitem = app_menu_item_new(appinfo);

		/* TODO insert it at the right position (alphabetically by application name) */
		g_menu_insert_section (menu, 2,
				       app_menu_item_get_name (menuitem),
				       app_menu_item_get_menu (menuitem));

		g_hash_table_insert (applications, g_strdup (desktop_file), menuitem);
	}

	g_object_unref (appinfo);
}

/* This function turns a specific desktop id into a menu
   item and registers it appropriately with everyone */
static gboolean
build_launcher (gpointer data)
{
	gchar *desktop_id = data;

	add_application (desktop_id, NULL);

	g_free (desktop_id);
	return FALSE;
}

/* This function goes through all the launchers that we're
   supposed to be grabbing and decides to show turn them
   into menu items or not.  It doens't do the work, but it
   makes the decision. */
static gboolean
build_launchers (gpointer data)
{
	gchar **applications = g_settings_get_strv (settings, "applications");
	gchar **app;

	g_return_val_if_fail (applications != NULL, FALSE);

	for (app = applications; *app; app++)
	{
		g_idle_add(build_launcher, g_strdup (*app));
	}

	g_strfreev (applications);
	return FALSE;
}

static void 
service_shutdown (IndicatorService * service, gpointer user_data)
{
	g_warning("Shutting down service!");
	g_main_loop_quit(mainloop);
	return;
}

static void
clear_action_activate (GSimpleAction *simple,
		       GVariant *param,
		       gpointer user_data)
{
	MessageServiceDbus *msg_service = user_data;
	message_service_dbus_set_attention(msg_service, FALSE);
}

static void
clear_action_handler (MessageServiceDbus *msd,
		      gboolean attention,
		      gpointer user_data)
{
	GSimpleAction *action = user_data;
	g_simple_action_set_enabled (action, attention);
}

static void
register_application (MessageServiceDbus *msd,
		      const gchar *desktop_id,
		      const gchar *menu_path,
		      gpointer user_data)
{
	gchar **applications = g_settings_get_strv (settings, "applications");
	gchar **app;

	for (app = applications; *app; app++) {
		if (!g_strcmp0 (desktop_id, *app))
			break;
	}

	if (*app == NULL) {
		GVariantBuilder builder;

		g_variant_builder_init (&builder, (GVariantType *)"as");
		for (app = applications; *app; app++)
			g_variant_builder_add (&builder, "s", *app);
		g_variant_builder_add (&builder, "s", desktop_id);

		g_settings_set_value (settings, "applications",
				      g_variant_builder_end (&builder));

		add_application (desktop_id, menu_path);
	}

	g_strfreev (applications);
}

static void
unregister_application (MessageServiceDbus *msd,
			const gchar *desktop_id,
			gpointer user_data)
{
	gchar **applications = g_settings_get_strv (settings, "applications");
	gchar **app;
	GVariantBuilder builder;

	g_variant_builder_init (&builder, (GVariantType *)"as");
	for (app = applications; *app; app++) {
		if (g_strcmp0 (desktop_id, *app))
			g_variant_builder_add (&builder, "s", *app);
	}

	g_settings_set_value (settings, "applications",
			      g_variant_builder_end (&builder));

	g_strfreev (applications);
}

int
main (int argc, char ** argv)
{
	GError *error = NULL;
	GActionEntry entries[] = {
		{ "status", NULL, "s", "'offline'", NULL },
		{ "clear", clear_action_activate }
	};
	GMenuModel *status_items;

	/* Glib init */
	g_type_init();

	/* Create the Indicator Service interface */
	service = indicator_service_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);
	g_signal_connect(service, INDICATOR_SERVICE_SIGNAL_SHUTDOWN, G_CALLBACK(service_shutdown), NULL);

	/* Setting up i18n and gettext.  Apparently, we need
	   all of these. */
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	/* Bring up the service DBus interface */
	dbus_interface = message_service_dbus_new();

	bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (!bus) {
		g_warning ("unable to connect to the session bus: %s", error->message);
		g_error_free (error);
		return 1;
	}

	actions = g_simple_action_group_new ();
	g_simple_action_group_add_entries (actions, entries, G_N_ELEMENTS (entries), NULL);
	g_dbus_connection_export_action_group (bus, INDICATOR_MESSAGES_DBUS_OBJECT,
                                               G_ACTION_GROUP (actions), &error);
	if (error) {
		g_warning ("unable to export action group on dbus: %s", error->message);
		g_error_free (error);
		return 1;
	}

	g_signal_connect (dbus_interface, MESSAGE_SERVICE_DBUS_SIGNAL_ATTENTION_CHANGED,
			  G_CALLBACK(clear_action_handler),
			  g_action_map_lookup_action (G_ACTION_MAP (actions), "clear"));

	g_signal_connect (dbus_interface, MESSAGE_SERVICE_DBUS_SIGNAL_REGISTER_APPLICATION,
			  G_CALLBACK (register_application), NULL);
	g_signal_connect (dbus_interface, MESSAGE_SERVICE_DBUS_SIGNAL_UNREGISTER_APPLICATION,
			  G_CALLBACK (unregister_application), NULL);

	status_items = status_items_build (g_action_map_lookup_action (G_ACTION_MAP (actions), "status"));

	menu = g_menu_new ();
	g_menu_append_section (menu, _("Status"), status_items);
	g_menu_append (menu, _("Clear"), "clear");

	g_dbus_connection_export_menu_model (bus, INDICATOR_MESSAGES_DBUS_OBJECT,
					     G_MENU_MODEL (menu), &error);
	if (error) {
		g_warning ("unable to export menu on dbus: %s", error->message);
		g_error_free (error);
		return 1;
	}

	settings = g_settings_new ("com.canonical.indicator.messages");

	applications = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	g_idle_add(build_launchers, NULL);

	/* Let's run a mainloop */
	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	/* Clean up */
	status_items_cleanup();
	g_object_unref (settings);
	g_hash_table_unref (applications);
	return 0;
}
