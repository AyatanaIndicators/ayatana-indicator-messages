/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>
    Lars Uebernickel <lars.uebernickel@canonical.com>

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

#include "dbus-data.h"
#include "gactionmuxer.h"
#include "gsettingsstrv.h"
#include "gmenuutils.h"
#include "indicator-messages-service.h"
#include "indicator-messages-application.h"

#define NUM_STATUSES 5

static GHashTable *applications;

static IndicatorMessagesService *messages_service;
static GSimpleActionGroup *actions;
static GActionMuxer *action_muxer;
static GMenu *toplevel_menu;
static GMenu *menu;
static GMenu *messages_section;
static GMenu *sources_section;
static GSettings *settings;
static gboolean draws_attention;
static const gchar *global_status[6]; /* max 5: available, away, busy, invisible, offline */

static gchar *
indicator_messages_get_icon_name ()
{
	GString *name;
	GIcon *icon;
	gchar *iconstr;

	name = g_string_new ("indicator-messages");

	if (global_status[0] != NULL)
	{
		if (global_status[1] != NULL)
			g_string_append (name, "-mixed");
		else
			g_string_append_printf (name, "-%s", global_status[0]);
	}

	if (draws_attention)
		g_string_append (name, "-new");

	icon = g_themed_icon_new (name->str);
	g_themed_icon_append_name (G_THEMED_ICON (icon),
				   draws_attention ? "indicator-messages-new"
						   : "indicator-messages");

	iconstr = g_icon_to_string (icon);

	g_object_unref (icon);
	g_string_free (name, TRUE);

	return iconstr;
}

static void 
service_shutdown (IndicatorService * service, gpointer user_data)
{
	GMainLoop *mainloop = user_data;

	g_warning("Shutting down service!");
	g_main_loop_quit(mainloop);
}

static void
clear_action_activate (GSimpleAction *simple,
		       GVariant *param,
		       gpointer user_data)
{
	/* TODO */
}

static void
status_action_activate (GSimpleAction *action,
			GVariant *parameter,
			gpointer user_data)
{
	const gchar *status;

	status = g_variant_get_string (parameter, NULL);

	indicator_messages_service_emit_status_changed (messages_service, status);
}

static void
sources_listed (GObject      *source_object,
		GAsyncResult *result,
		gpointer      user_data)
{
	gchar *app_id = user_data;
	GVariant *sources;
	GVariantIter iter;
	const gchar *id;
	const gchar *label;
	const gchar *iconstr;
	guint32 count;
	gint64 time;
	const gchar *string;
	gboolean draws_attention;
	GError *error = NULL;

	if (!indicator_messages_application_call_list_sources_finish (INDICATOR_MESSAGES_APPLICATION (source_object),
								      &sources, result, &error))
	{
		g_warning ("could not fetch the list of sources: %s", error->message);
		g_error_free (error);
		g_free (app_id);
		return;
	}

	g_variant_iter_init (&iter, sources);
	while (g_variant_iter_next (&iter, "(&s&s&sux&sb)", &id, &label, &iconstr, &count,
							    &time, &string, &draws_attention))
	{
		GMenuItem *item;
		GActionGroup *app_actions;
		GSimpleAction *action;
		gchar *action_name;

		app_actions = g_action_muxer_get_group (action_muxer, app_id);
		g_assert (app_actions);
		action = g_simple_action_new_stateful (id, NULL, g_variant_new_uint32 (count));
		g_simple_action_group_insert (G_SIMPLE_ACTION_GROUP (app_actions), G_ACTION (action));

		action_name = g_strconcat (app_id, ".", id, NULL);
		item = g_menu_item_new (label, action_name);
		g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.messages.sourceitem");
		if (iconstr && *iconstr)
			g_menu_item_set_attribute (item, "x-canonical-icon", "s", iconstr);
		g_menu_append_item (sources_section, item);

		g_object_unref (action);
		g_free (action_name);
		g_object_unref (item);
	}

	g_variant_unref (sources);
	g_free (app_id);
}

static void
messages_listed (GObject      *source_object,
		 GAsyncResult *result,
		 gpointer      user_data)
{
	gchar *app_id = user_data;
	GVariant *messages;
	GError *error = NULL;
	GVariantIter iter;
	const gchar *id;
	const gchar *iconstr;
	const gchar *title;
	const gchar *subtitle;
	const gchar *body;
	gint64 time;
	gboolean draws_attention;

	if (!indicator_messages_application_call_list_messages_finish (INDICATOR_MESSAGES_APPLICATION (source_object),
								       &messages, result, &error))
	{
		g_warning ("could not fetch the list of messages: %s", error->message);
		g_error_free (error);
		g_free (app_id);
		return;
	}

	g_variant_iter_init (&iter, messages);
	while (g_variant_iter_next (&iter, "(&s&s&s&s&sxb)", &id, &iconstr, &title, &subtitle, &body,
							     &time, &draws_attention))
	{
		GMenuItem *item;
		GActionGroup *app_actions;
		GSimpleAction *action;
		gchar *action_name;

		app_actions = g_action_muxer_get_group (action_muxer, app_id);
		g_assert (app_actions);
		action = g_simple_action_new (id, NULL);
		g_simple_action_group_insert (G_SIMPLE_ACTION_GROUP (app_actions), G_ACTION (action));

		action_name = g_strconcat (app_id, ".", id, NULL);
		item = g_menu_item_new (NULL, action_name);
		g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.messages.messageitem");
		g_menu_item_set_attribute (item, "x-canonical-message-id", "s", id);
		g_menu_item_set_attribute (item, "x-canonical-sender", "s", title);
		g_menu_item_set_attribute (item, "x-canonical-subject", "s", subtitle);
		g_menu_item_set_attribute (item, "x-canonical-body", "s", body);
		g_menu_item_set_attribute (item, "x-canonical-time", "x", time);
		if (iconstr && *iconstr)
			g_menu_item_set_attribute (item, "x-canonical-avatar", "s", iconstr);
		g_menu_append_item (messages_section, item);

		g_object_unref (action);
		g_free (action_name);
		g_object_unref (item);
	}

	g_variant_unref (messages);
	g_free (app_id);
}

static void
app_proxy_created (GObject      *source_object,
		   GAsyncResult *result,
		   gpointer      user_data)
{
	gchar *desktop_id = user_data;
	IndicatorMessagesApplication *app_proxy;
	GError *error = NULL;

	app_proxy = indicator_messages_application_proxy_new_finish (result, &error);
	if (!app_proxy) {
		g_warning ("could not create application proxy: %s", error->message);
		g_error_free (error);
		return;
	}

	/* hash table takes ownership of desktop_id and app_proxy */
	g_hash_table_insert (applications, desktop_id, app_proxy);

	indicator_messages_application_call_list_sources (app_proxy, NULL, sources_listed, g_strdup (desktop_id));
	indicator_messages_application_call_list_messages (app_proxy, NULL, messages_listed, g_strdup (desktop_id));
}

static void
register_application (IndicatorMessagesService *service,
		      GDBusMethodInvocation *invocation,
		      const gchar *desktop_id,
		      const gchar *menu_path,
		      gpointer user_data)
{
	GDBusConnection *bus;
	const gchar *sender;
	GSimpleActionGroup *app_actions;

	if (g_hash_table_lookup (applications, desktop_id)) {
		g_warning ("application with id '%s' already exists", desktop_id);
		g_dbus_method_invocation_return_dbus_error (invocation,
							    "com.canonical.indicator.messages.ApplicationAlreadyRegistered",
							    "another application is already registered with this id");
		return;
	}

	bus = g_dbus_interface_skeleton_get_connection (G_DBUS_INTERFACE_SKELETON (service));
	sender = g_dbus_method_invocation_get_sender (invocation);

	indicator_messages_application_proxy_new (bus, G_DBUS_PROXY_FLAGS_NONE,
						  sender, menu_path, NULL,
						  app_proxy_created, g_strdup (desktop_id));

	app_actions = g_simple_action_group_new ();
	g_action_muxer_insert (action_muxer, desktop_id, G_ACTION_GROUP (app_actions));

	g_settings_strv_append_unique (settings, "applications", desktop_id);

	indicator_messages_service_complete_register_application (service, invocation);

	g_object_unref (app_actions);
}

static void
unregister_application (IndicatorMessagesService *service,
			GDBusMethodInvocation *invocation,
			const gchar *desktop_id,
			gpointer user_data)
{
	if (g_hash_table_remove (applications, desktop_id)) {

		/* TODO remove menu items that refer to this application */
		g_action_muxer_remove (action_muxer, desktop_id);

		g_settings_strv_remove (settings, "applications", desktop_id);
	}

	indicator_messages_service_complete_unregister_application (service, invocation);
}

static GSimpleActionGroup *
create_action_group (void)
{
	GSimpleActionGroup *actions;
	GSimpleAction *messages;
	GSimpleAction *clear;
	GSimpleAction *status;
	const gchar *default_status[] = { "offline", NULL };
	gchar *icon;

	actions = g_simple_action_group_new ();

	/* state of the messages action is its icon name */
	icon = indicator_messages_get_icon_name ();
	messages = g_simple_action_new_stateful ("messages", G_VARIANT_TYPE ("s"),
						 g_variant_new_string (icon));

	status = g_simple_action_new_stateful ("status", G_VARIANT_TYPE ("s"),
					       g_variant_new_strv (default_status, -1));
	g_signal_connect (status, "activate", G_CALLBACK (status_action_activate), NULL);

	clear = g_simple_action_new ("clear", NULL);
	g_simple_action_set_enabled (clear, FALSE);
	g_signal_connect (clear, "activate", G_CALLBACK (clear_action_activate), NULL);

	g_simple_action_group_insert (actions, G_ACTION (messages));
	g_simple_action_group_insert (actions, G_ACTION (status));
	g_simple_action_group_insert (actions, G_ACTION (clear));

	g_free (icon);
	return actions;
}

static void
got_bus (GObject *object,
	 GAsyncResult * res,
	 gpointer user_data)
{
	GDBusConnection *bus;
	GError *error = NULL;

	bus = g_bus_get_finish (res, &error);
	if (!bus) {
		g_warning ("unable to connect to the session bus: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_connection_export_action_group (bus, INDICATOR_MESSAGES_DBUS_OBJECT,
                                               G_ACTION_GROUP (action_muxer), &error);
	if (error) {
		g_warning ("unable to export action group on dbus: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_connection_export_menu_model (bus, INDICATOR_MESSAGES_DBUS_OBJECT "/phone",
					     G_MENU_MODEL (toplevel_menu), &error);
	if (error) {
		g_warning ("unable to export menu on dbus: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (messages_service),
					  bus, INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
					  &error);
	if (error) {
		g_warning ("unable to export messages service on dbus: %s", error->message);
		g_error_free (error);
		return;
	}

	g_object_unref (bus);
}

int
main (int argc, char ** argv)
{
	GMainLoop * mainloop = NULL;
	IndicatorService * service = NULL;

	/* Glib init */
	g_type_init();

	mainloop = g_main_loop_new (NULL, FALSE);

	/* Create the Indicator Service interface */
	service = indicator_service_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);
	g_signal_connect(service, INDICATOR_SERVICE_SIGNAL_SHUTDOWN, G_CALLBACK(service_shutdown), mainloop);

	/* Setting up i18n and gettext.  Apparently, we need
	   all of these. */
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	/* Bring up the service DBus interface */
	messages_service = indicator_messages_service_skeleton_new ();

	g_bus_get (G_BUS_TYPE_SESSION, NULL, got_bus, NULL);

	actions = create_action_group ();

	action_muxer = g_action_muxer_new ();
	g_action_muxer_insert (action_muxer, NULL, G_ACTION_GROUP (actions));

	g_signal_connect (messages_service, "handle-register-application",
			  G_CALLBACK (register_application), NULL);
	g_signal_connect (messages_service, "handle-unregister-application",
			  G_CALLBACK (unregister_application), NULL);

	menu = g_menu_new ();
	sources_section = g_menu_new ();
	messages_section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL (sources_section));
	g_menu_append_section (menu, NULL, G_MENU_MODEL (messages_section));

	toplevel_menu = g_menu_new ();
	g_menu_append_submenu (toplevel_menu, NULL, G_MENU_MODEL (menu));

	settings = g_settings_new ("com.canonical.indicator.messages");

	applications = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	g_main_loop_run(mainloop);

	/* Clean up */
	g_object_unref (messages_service);
	g_object_unref (settings);
	g_hash_table_unref (applications);
	return 0;
}
