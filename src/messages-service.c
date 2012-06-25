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

#include <config.h>
#include <locale.h>
#include <libindicator/indicator-service.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include "app-section.h"
#include "dbus-data.h"
#include "messages-service-dbus.h"
#include "gactionmuxer.h"

static IndicatorService * service = NULL;
static GHashTable *applications;

static GDBusConnection *bus;
static GSimpleActionGroup *actions;
static GActionMuxer *action_muxer;
static GMenu *menu;
static GSettings *settings;
static GMainLoop * mainloop = NULL;

static MessageServiceDbus * dbus_interface = NULL;


static gchar *
g_app_info_get_simple_id (GAppInfo *appinfo)
{
	const gchar *id;

	id = g_app_info_get_id (appinfo);
	if (!id)
		return NULL;

	if (g_str_has_suffix (id, ".desktop"))
		return g_strndup (id, strlen (id) - 8);
	else
		return g_strdup (id);
}

static void
actions_changed (GObject *object,
		 GParamSpec *pspec,
		 gpointer user_data)
{
	AppSection *section = APP_SECTION (object);
	gchar *id;
	GActionGroup *actions;

	id = g_app_info_get_simple_id (app_section_get_app_info (section));
	actions = app_section_get_actions (section);

	g_action_muxer_insert (action_muxer, id, actions);
	g_free (id);
}

static AppSection *
add_application (const gchar *desktop_id)
{
	GDesktopAppInfo *appinfo;
	gchar *id;
	AppSection *section;

	appinfo = g_desktop_app_info_new (desktop_id);
	if (!appinfo) {
		g_warning ("could not add '%s', there's no desktop file with that id", desktop_id);
		return NULL;
	}

	id = g_app_info_get_simple_id (G_APP_INFO (appinfo));
	section = g_hash_table_lookup (applications, id);

	if (!section) {
		GMenuItem *menuitem;

		section = app_section_new(appinfo);
		g_hash_table_insert (applications, g_strdup (id), section);

		g_action_muxer_insert (action_muxer, id, app_section_get_actions (section));
		g_signal_connect (section, "notify::actions",
				  G_CALLBACK (actions_changed), NULL);

		/* TODO insert it at the right position (alphabetically by application name) */
		menuitem = g_menu_item_new_section (NULL, app_section_get_menu (section));
		g_menu_item_set_attribute (menuitem, "action-namespace", "s", id);
		g_menu_insert_item (menu, 2, menuitem);
		g_object_unref (menuitem);
	}

	g_free (id);
	g_object_unref (appinfo);
	return section;
}

/* g_menu_model_find_section:
 *
 * @Returns the index of the first menu item that is linked to #section, or -1
 * if there's no such item.
 */
static int
g_menu_find_section (GMenu *menu,
		     GMenuModel *section)
{
	int n_items = g_menu_model_get_n_items (G_MENU_MODEL (menu));
	int i;

	for (i = 0; i < n_items; i++) {
		if (section == g_menu_model_get_item_link (G_MENU_MODEL (menu), i, G_MENU_LINK_SECTION))
			return i;
	}

	return -1;
}

static void
remove_application (const char *desktop_id)
{
	GDesktopAppInfo *appinfo;
	gchar *id;
	AppSection *section;

	appinfo = g_desktop_app_info_new (desktop_id);
	if (!appinfo) {
		g_warning ("could not remove '%s', there's no desktop file with that id", desktop_id);
		return;
	}

	id = g_app_info_get_simple_id (G_APP_INFO (appinfo));

	section = g_hash_table_lookup (applications, id);
	if (section) {
		int pos = g_menu_find_section (menu, app_section_get_menu (section));
		if (pos >= 0)
			g_menu_remove (menu, pos);
		g_action_muxer_remove (action_muxer, id);
	}
	else {
		g_warning ("could not remove '%s', it's not registered", desktop_id);
	}
	
	g_hash_table_remove (applications, id);
	g_free (id);
	g_object_unref (appinfo);
}

/* This function turns a specific desktop id into a menu
   item and registers it appropriately with everyone */
static gboolean
build_launcher (gpointer data)
{
	gchar *desktop_id = data;

	add_application (desktop_id);

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
radio_item_activate (GSimpleAction *action,
		     GVariant *parameter,
		     gpointer user_data)
{
	g_action_change_state (G_ACTION (action), parameter);
}

static void
change_status (GSimpleAction *action,
	       GVariant *value,
	       gpointer user_data)
{
	g_message ("changing status to %s", g_variant_get_string (value, NULL));
}

static void
clear_action_handler (MessageServiceDbus *msd,
		      gboolean attention,
		      gpointer user_data)
{
	GSimpleAction *action = user_data;
	g_simple_action_set_enabled (action, attention);
}

static gint
g_strv_find (gchar **str_array,
	     const gchar *key)
{
	gchar **it;

	g_return_val_if_fail (str_array != NULL, -1);

	for (it = str_array; *it; it++) {
		if (!g_strcmp0 (key, *it))
			return it - str_array;
	}

	return -1;
}

static void
register_application (MessageServiceDbus *msd,
		      const gchar *sender,
		      const gchar *desktop_id,
		      const gchar *menu_path,
		      gpointer user_data)
{
	AppSection *section;
	gchar **applications;

	section = add_application (desktop_id);
	if (!section)
		return;

	app_section_set_object_path (section, bus, sender, menu_path);

	/* remember this application in the settings key */
	applications = g_settings_get_strv (settings, "applications");
	if (g_strv_find (applications, desktop_id) < 0) {
		GVariantBuilder builder;
		gchar **app;

		g_variant_builder_init (&builder, (GVariantType *)"as");
		for (app = applications; *app; app++)
			g_variant_builder_add (&builder, "s", *app);
		g_variant_builder_add (&builder, "s", desktop_id);

		g_settings_set_value (settings, "applications",
				      g_variant_builder_end (&builder));
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

	remove_application (desktop_id);

	g_strfreev (applications);
}

static void
g_menu_append_with_icon (GMenu *menu,
			 const gchar *label,
			 const gchar *icon_name,
			 const gchar *detailed_action)
{
	GMenuItem *item;

	item = g_menu_item_new (label, detailed_action);
	g_menu_item_set_attribute (item, INDICATOR_MENU_ATTRIBUTE_ICON_NAME, "s", icon_name);

	g_menu_append_item (menu, item);
	g_object_unref (item);
}

GMenuModel *
create_status_section ()
{
	GMenu *menu;

	menu = g_menu_new ();
	g_menu_append_with_icon (menu, _("Available"), "user-available", "status::available");
	g_menu_append_with_icon (menu, _("Away"),      "user-away",      "status::away");
	g_menu_append_with_icon (menu, _("Busy"),      "user-busy",      "status::busy");
	g_menu_append_with_icon (menu, _("Invisible"), "user-invisible", "status::invisible");
	g_menu_append_with_icon (menu, _("Offline"),   "user-offline",   "status::offline");

	return G_MENU_MODEL (menu);
}

int
main (int argc, char ** argv)
{
	GError *error = NULL;
	GActionEntry entries[] = {
		{ "status", radio_item_activate, "s", "'offline'", change_status },
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
	g_simple_action_group_add_entries (actions, entries, G_N_ELEMENTS (entries), dbus_interface);

	action_muxer = g_action_muxer_new ();
	g_action_muxer_insert (action_muxer, NULL, G_ACTION_GROUP (actions));
	g_dbus_connection_export_action_group (bus, INDICATOR_MESSAGES_DBUS_OBJECT,
                                               G_ACTION_GROUP (action_muxer), &error);
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

	menu = g_menu_new ();
	status_items = create_status_section ();
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
	g_object_unref (status_items);
	g_object_unref (settings);
	g_hash_table_unref (applications);
	return 0;
}
