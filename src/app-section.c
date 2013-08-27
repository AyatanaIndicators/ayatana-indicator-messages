/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2012 Canonical Ltd.

Authors:
    Lars Uebernickel <lars.uebernickel@canonical.com>
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
#include "gmenuutils.h"
#include "gactionmuxer.h"
#include "indicator-messages-application.h"

struct _AppSectionPrivate
{
	GDesktopAppInfo * appinfo;
	GFileMonitor *desktop_file_monitor;

	IndicatorDesktopShortcuts * ids;

	GCancellable *app_proxy_cancellable;
	IndicatorMessagesApplication *app_proxy;

	GMenu *menu;
	GMenu *source_menu;

	GSimpleActionGroup *static_shortcuts;
	GSimpleActionGroup *source_actions;
	GActionMuxer *muxer;

	gboolean draws_attention;
	gboolean uses_chat_status;
	gchar *chat_status;

	guint name_watch_id;
};

enum {
	PROP_0,
	PROP_APPINFO,
	PROP_ACTIONS,
	PROP_DRAWS_ATTENTION,
	PROP_USES_CHAT_STATUS,
	PROP_CHAT_STATUS,
	NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];
static guint destroy_signal;

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
static void app_section_finalize     (GObject *object);
static void activate_cb              (GSimpleAction *action,
				      GVariant *param,
				      gpointer userdata);
static void launch_action_change_state (GSimpleAction *action,
					GVariant      *value,
					gpointer       user_data);
static void app_section_set_app_info (AppSection *self,
				      GDesktopAppInfo *appinfo);
static void	desktop_file_changed_cb		(GFileMonitor      *monitor,
						 GFile             *file,
						 GFile             *other_file,
						 GFileMonitorEvent  event,
						 gpointer           user_data);

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
	object_class->finalize = app_section_finalize;

	properties[PROP_APPINFO] = g_param_spec_object ("app-info",
							"AppInfo",
							"The GAppInfo for the app that this menu represents",
							G_TYPE_APP_INFO,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_ACTIONS] = g_param_spec_object ("actions",
							"Actions",
							"The actions exported by this application",
							G_TYPE_ACTION_GROUP,
							G_PARAM_READABLE);

	properties[PROP_DRAWS_ATTENTION] = g_param_spec_boolean ("draws-attention",
								 "Draws attention",
								 "Whether the section currently draws attention",
								 FALSE,
								 G_PARAM_READABLE);

	properties[PROP_USES_CHAT_STATUS] = g_param_spec_boolean ("uses-chat-status",
								  "Uses chat status",
								  "Whether the section uses the global chat status",
								  FALSE,
								  G_PARAM_READABLE);

	properties[PROP_CHAT_STATUS] = g_param_spec_string ("chat-status",
							    "Chat status",
							    "Current chat status of the application",
							    NULL,
							    G_PARAM_READWRITE |
							    G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

	destroy_signal = g_signal_new ("destroy",
				       APP_SECTION_TYPE,
				       G_SIGNAL_RUN_FIRST,
				       0,
				       NULL, NULL,
				       g_cclosure_marshal_VOID__VOID,
				       G_TYPE_NONE, 0);
}

static void
app_section_init (AppSection *self)
{
	AppSectionPrivate *priv;
	GMenuItem *item;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
						  APP_SECTION_TYPE,
						  AppSectionPrivate);
	priv = self->priv;

	priv->appinfo = NULL;

	priv->menu = g_menu_new ();

	priv->source_menu = g_menu_new ();
	item = g_menu_item_new_section (NULL, G_MENU_MODEL (priv->source_menu));
	g_menu_item_set_attribute (item, "action-namespace", "s", "source");
	g_menu_append_item (priv->menu, item);
	g_object_unref (item);

	priv->static_shortcuts = g_simple_action_group_new ();
	priv->source_actions = g_simple_action_group_new ();

	priv->muxer = g_action_muxer_new ();
	g_action_muxer_insert (priv->muxer, NULL, G_ACTION_GROUP (priv->static_shortcuts));
	g_action_muxer_insert (priv->muxer, "source", G_ACTION_GROUP (priv->source_actions));

	priv->draws_attention = FALSE;

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

	case PROP_DRAWS_ATTENTION:
		g_value_set_boolean (value, app_section_get_draws_attention (self));
		break;

	case PROP_USES_CHAT_STATUS:
		g_value_set_boolean (value, app_section_get_uses_chat_status (self));
		break;

	case PROP_CHAT_STATUS:
		g_value_set_string (value, app_section_get_status (self));
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

	case PROP_CHAT_STATUS:
		app_section_set_status (self, g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
app_section_dispose (GObject *object)
{
	AppSection * self = APP_SECTION(object);
	AppSectionPrivate * priv = self->priv;

	if (priv->app_proxy_cancellable) {
		g_cancellable_cancel (priv->app_proxy_cancellable);
		g_clear_object (&priv->app_proxy_cancellable);
	}

	if (priv->desktop_file_monitor) {
		g_signal_handlers_disconnect_by_func (priv->desktop_file_monitor, desktop_file_changed_cb, self);
		g_clear_object (&priv->desktop_file_monitor);
	}

	g_clear_object (&priv->app_proxy);

	g_clear_object (&priv->menu);
	g_clear_object (&priv->source_menu);
	g_clear_object (&priv->static_shortcuts);
	g_clear_object (&priv->source_actions);

	if (priv->name_watch_id) {
		g_bus_unwatch_name (priv->name_watch_id);
		priv->name_watch_id = 0;
	}

	g_clear_object (&priv->muxer);

	g_clear_object (&priv->ids);
	g_clear_object (&priv->appinfo);

	G_OBJECT_CLASS (app_section_parent_class)->dispose (object);
}

static void
app_section_finalize (GObject *object)
{
	AppSection * self = APP_SECTION(object);

	g_free (self->priv->chat_status);

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
	AppSectionPrivate * priv = mi->priv;

	g_return_if_fail(priv->ids != NULL);

	if (!indicator_desktop_shortcuts_nick_exec_with_context(priv->ids, nick, NULL)) {
		g_warning("Unable to execute nick '%s' for desktop file '%s'",
			  nick, g_desktop_app_info_get_filename (priv->appinfo));
	}

	return;
}

static void
keyfile_loaded (GObject *source_object,
		GAsyncResult *result,
		gpointer user_data)
{
	AppSection *self = user_data;
	gchar *contents;
	gsize length;
	GKeyFile *keyfile;
	GError *error = NULL;

	if (!g_file_load_contents_finish (G_FILE (source_object), result,
					 &contents, &length, NULL, &error)) {
		g_warning ("could not read key file: %s", error->message);
		g_error_free (error);
		return;
	}

	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_data (keyfile, contents, length, 0, &error)) {
		g_warning ("could not read key file: %s", error->message);
		g_error_free (error);
		goto out;
	}

	self->priv->uses_chat_status = g_key_file_get_boolean (keyfile,
							       G_KEY_FILE_DESKTOP_GROUP,
							       "X-MessagingMenu-UsesChatSection",
							       &error);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USES_CHAT_STATUS]);

	if (error) {
		if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			g_warning ("could not read X-MessagingMenu-UsesChatSection: %s",
				   error->message);
		}
		g_error_free (error);
		goto out;
	}

out:
	g_key_file_free (keyfile);
	g_free (contents);
}

static void
g_menu_clear (GMenu *menu)
{
	gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (menu));

	while (n_items--)
		g_menu_remove (menu, 0);
}

static void
g_simple_action_group_clear (GSimpleActionGroup *group)
{
	gchar **actions;
	gchar **it;

	actions = g_action_group_list_actions (G_ACTION_GROUP (group));
	for (it = actions; *it; it++)
		g_action_map_remove_action (G_ACTION_MAP(group), *it);

	g_strfreev (actions);
}

static void
app_section_update_menu (AppSection *self)
{
	AppSectionPrivate *priv = self->priv;
	GSimpleAction *launch;
	GFile *keyfile;
	GMenuItem *item;
	gchar *iconstr;
	gboolean is_running;

	g_menu_clear (priv->menu);
	g_simple_action_group_clear (priv->static_shortcuts);

	is_running = priv->name_watch_id > 0;
	launch = g_simple_action_new_stateful ("launch", G_VARIANT_TYPE_UINT32, g_variant_new_boolean (is_running));
	g_signal_connect (launch, "activate", G_CALLBACK (activate_cb), self);
	g_signal_connect (launch, "change-state", G_CALLBACK (launch_action_change_state), self);
	g_action_map_add_action (G_ACTION_MAP(priv->static_shortcuts), G_ACTION (launch));

	item = g_menu_item_new (g_app_info_get_name (G_APP_INFO (priv->appinfo)), "launch");
	g_menu_item_set_attribute (item, "x-canonical-type", "s", "ImAppMenuItem");
	iconstr = g_icon_to_string (g_app_info_get_icon (G_APP_INFO (priv->appinfo)));
	if (iconstr != NULL) {
		g_menu_item_set_attribute (item, "x-canonical-icon", "s", iconstr);
	}
	g_free (iconstr);

	g_menu_append_item (priv->menu, item);
	g_object_unref (item);

	/* Start to build static shortcuts */
	priv->ids = indicator_desktop_shortcuts_new(g_desktop_app_info_get_filename (priv->appinfo), "Messaging Menu");
	const gchar ** nicks = indicator_desktop_shortcuts_get_nicks(priv->ids);
	gint i;
	for (i = 0; nicks[i] != NULL; i++) {
		gchar *name;
		GSimpleAction *action;
		GMenuItem *item;

		name = indicator_desktop_shortcuts_nick_get_name(priv->ids, nicks[i]);

		action = g_simple_action_new (nicks[i], G_VARIANT_TYPE_UINT32);
		g_signal_connect(action, "activate", G_CALLBACK (nick_activate_cb), self);
		g_action_map_add_action (G_ACTION_MAP(priv->static_shortcuts), G_ACTION (action));
		g_object_unref (action);

		item = g_menu_item_new (name, nicks[i]);
		g_menu_item_set_attribute (item, "x-canonical-type", "s", "IdoMenuItem");
		g_menu_item_set_attribute (item, "x-canonical-icon", "s", ""); /* empty to get indentation */
		g_menu_append_item (priv->menu, item);

		g_object_unref (item);
		g_free(name);
	}

	item = g_menu_item_new_section (NULL, G_MENU_MODEL (priv->source_menu));
	g_menu_item_set_attribute (item, "action-namespace", "s", "source");
	g_menu_append_item (priv->menu, item);
	g_object_unref (item);

	keyfile = g_file_new_for_path (g_desktop_app_info_get_filename (priv->appinfo));
	g_file_load_contents_async (keyfile, NULL, keyfile_loaded, self);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);

	g_object_unref (keyfile);
	g_object_unref (launch);
}

static void
desktop_file_changed_cb (GFileMonitor      *monitor,
			 GFile             *file,
			 GFile             *other_file,
			 GFileMonitorEvent  event,
			 gpointer           user_data)
{
	AppSection *self = user_data;

	if (event == G_FILE_MONITOR_EVENT_CHANGED) {
		app_section_update_menu (self);
	}
	else if (event == G_FILE_MONITOR_EVENT_DELETED ||
		 event == G_FILE_MONITOR_EVENT_UNMOUNTED) {
		g_signal_emit (self, destroy_signal, 0);
	}
}

static void
app_section_set_app_info (AppSection *self,
			  GDesktopAppInfo *appinfo)
{
	AppSectionPrivate *priv = self->priv;
	GFile *desktop_file;
	GError *error = NULL;

	g_return_if_fail (priv->appinfo == NULL);
	g_return_if_fail (priv->desktop_file_monitor == NULL);

	if (appinfo == NULL) {
		g_warning ("appinfo must not be NULL");
		return;
	}

	priv->appinfo = g_object_ref (appinfo);

	desktop_file = g_file_new_for_path (g_desktop_app_info_get_filename (appinfo));
	priv->desktop_file_monitor = g_file_monitor (desktop_file, G_FILE_MONITOR_SEND_MOVED, NULL, &error);
	if (priv->desktop_file_monitor == NULL) {
		g_warning ("unable to watch desktop file: %s", error->message);
		g_error_free (error);
	}
	g_signal_connect (priv->desktop_file_monitor, "changed",
			  G_CALLBACK (desktop_file_changed_cb), self);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_APPINFO]);

	app_section_update_menu (self);

	g_object_unref (desktop_file);
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
	AppSectionPrivate * priv = mi->priv;
	GError *error = NULL;

	if (!g_app_info_launch (G_APP_INFO (priv->appinfo), NULL, NULL, &error)) {
		g_warning("Unable to execute application for desktop file '%s': %s",
			  g_desktop_app_info_get_filename (priv->appinfo),
			  error->message);
		g_error_free (error);
	}
}

static void
launch_action_change_state (GSimpleAction *action,
			    GVariant      *value,
			    gpointer       user_data)
{
	g_simple_action_set_state (action, value);
}

const gchar *
app_section_get_desktop (AppSection * self)
{
	AppSectionPrivate * priv = self->priv;
	if (priv->appinfo)
		return g_desktop_app_info_get_filename (priv->appinfo);
	else
		return NULL;
}

GActionGroup *
app_section_get_actions (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;
	return G_ACTION_GROUP (priv->muxer);
}

GMenuModel *
app_section_get_menu (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;
	return G_MENU_MODEL (priv->menu);
}

GAppInfo *
app_section_get_app_info (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;
	return G_APP_INFO (priv->appinfo);
}

gboolean
app_section_get_draws_attention (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;
	return priv->draws_attention;
}

void
app_section_clear_draws_attention (AppSection *self)
{
	self->priv->draws_attention = FALSE;
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
}

static void
application_vanished (GDBusConnection *bus,
		      const gchar *name,
		      gpointer user_data)
{
	AppSection *self = user_data;

	app_section_unset_object_path (self);
}

static void
update_draws_attention (AppSection *self)
{
	AppSectionPrivate *priv = self->priv;
	gchar **actions;
	gchar **it;
	gboolean draws_attention = FALSE;

	actions = g_action_group_list_actions (G_ACTION_GROUP (priv->source_actions));

	for (it = actions; *it; it++) {
		GVariant *state;

		state = g_action_group_get_action_state (G_ACTION_GROUP (priv->source_actions), *it);
		if (state) {
			gboolean b;
			g_variant_get (state, "(uxsb)", NULL, NULL, NULL, &b);
			draws_attention = b || draws_attention;
			g_variant_unref (state);
		}

		if (draws_attention)
			break;
	}

	if (draws_attention != priv->draws_attention) {
		priv->draws_attention = draws_attention;
		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
	}

	g_strfreev (actions);
}

static void
remove_source (AppSection  *self,
	       const gchar *id)
{
	AppSectionPrivate *priv = self->priv;
	guint n_items;
	guint i;

	n_items = g_menu_model_get_n_items (G_MENU_MODEL (priv->source_menu));
	for (i = 0; i < n_items; i++) {
		gchar *action;
		gboolean found = FALSE;

		if (g_menu_model_get_item_attribute (G_MENU_MODEL (priv->source_menu), i,
						     G_MENU_ATTRIBUTE_ACTION, "s", &action)) {
			found = g_str_equal (action, id);
			g_free (action);
		}

		if (found) {
			g_menu_remove (priv->source_menu, i);
			break;
		}
	}

	g_action_map_remove_action (G_ACTION_MAP(priv->source_actions), id);
	update_draws_attention (self);
}

static void
source_action_activated (GSimpleAction *action,
			 GVariant      *parameter,
			 gpointer       user_data)
{
	AppSection *self = APP_SECTION (user_data);
	AppSectionPrivate *priv = APP_SECTION (user_data)->priv;

	g_return_if_fail (priv->app_proxy != NULL);

	indicator_messages_application_call_activate_source (priv->app_proxy,
							     g_action_get_name (G_ACTION (action)),
							     priv->app_proxy_cancellable,
							     NULL, NULL);

	remove_source (self, g_action_get_name (G_ACTION (action)));
}

static void
sources_listed (GObject      *source_object,
		GAsyncResult *result,
		gpointer      user_data)
{
	AppSection *self = user_data;
	AppSectionPrivate *priv = self->priv;
	GVariant *sources = NULL;
	GError *error = NULL;
	GVariantIter iter;
	const gchar *id;
	const gchar *label;
	const gchar *iconstr;
	guint32 count;
	gint64 time;
	const gchar *string;
	gboolean draws_attention;

	if (!indicator_messages_application_call_list_sources_finish (INDICATOR_MESSAGES_APPLICATION (source_object),
								      &sources, result, &error))
	{
		g_warning ("could not fetch the list of sources: %s", error->message);
		g_error_free (error);
		return;
	}

	g_menu_clear (priv->source_menu);
	g_simple_action_group_clear (priv->source_actions);
	priv->draws_attention = FALSE;

	g_variant_iter_init (&iter, sources);
	while (g_variant_iter_next (&iter, "(&s&s&sux&sb)", &id, &label, &iconstr,
				    &count, &time, &string, &draws_attention))
	{
		GVariant *state;
		GSimpleAction *action;
		GMenuItem *item;

		state = g_variant_new ("(uxsb)", count, time, string, draws_attention);
		action = g_simple_action_new_stateful (id, NULL, state);
		g_signal_connect (action, "activate", G_CALLBACK (source_action_activated), self);
		g_action_map_add_action (G_ACTION_MAP(priv->source_actions), G_ACTION (action));

		item = g_menu_item_new (label, id);
		g_menu_item_set_attribute (item, "x-canonical-type", "s", "ImSourceMenuItem");
		g_menu_append_item (priv->source_menu, item);

		priv->draws_attention = priv->draws_attention || draws_attention;

		g_object_unref (item);
		g_object_unref (action);
	}

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);

	g_variant_unref (sources);
}

static void
source_added (IndicatorMessagesApplication  *app,
	      const gchar		    *id,
	      const gchar		    *label,
	      const gchar		    *iconstr,
	      guint			     count,
	      gint64			     time,
	      const gchar		    *string,
	      gboolean			     draws_attention,
	      gpointer			     user_data)
{
	AppSection *self = user_data;
	AppSectionPrivate *priv = self->priv;
	GVariant *state;
	GSimpleAction *action;

	/* TODO put label and icon into the action as well */

	state = g_variant_new ("(uxsb)", count, time, string, draws_attention);
	action = g_simple_action_new_stateful (id, NULL, state);

	g_action_map_add_action (G_ACTION_MAP(priv->source_actions), G_ACTION (action));

	if (draws_attention && !priv->draws_attention) {
		priv->draws_attention = TRUE;
		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
	}

	g_object_unref (action);
}
static void
source_changed (IndicatorMessagesApplication  *app,
		const gchar		       *id,
		const gchar		       *label,
		const gchar		       *iconstr,
		guint				count,
		gint64				time,
		const gchar		       *string,
		gboolean			draws_attention,
		gpointer			user_data)
{
	AppSection *self = user_data;
	AppSectionPrivate *priv = self->priv;
	GVariant *state;

	/* TODO put label and icon into the action as well */

	state = g_variant_new ("(uxsb)", count, time, string, draws_attention);
	g_action_group_change_action_state (G_ACTION_GROUP (priv->source_actions), id, state);

	update_draws_attention (self);
}

static void
source_removed (IndicatorMessagesApplication *app,
		const gchar		     *id,
		gpointer		      user_data)
{
	AppSection *self = user_data;

	remove_source (self, id);
}

static void
app_proxy_created (GObject      *source_object,
		   GAsyncResult *result,
		   gpointer      user_data)
{
	AppSectionPrivate *priv = APP_SECTION (user_data)->priv;
	GError *error = NULL;

	priv->app_proxy = indicator_messages_application_proxy_new_finish (result, &error);
	if (!priv->app_proxy) {
		g_warning ("could not create application proxy: %s", error->message);
		g_error_free (error);
		return;
	}

	indicator_messages_application_call_list_sources (priv->app_proxy, priv->app_proxy_cancellable,
							  sources_listed, user_data);

	g_signal_connect (priv->app_proxy, "source-added", G_CALLBACK (source_added), user_data);
	g_signal_connect (priv->app_proxy, "source-changed", G_CALLBACK (source_changed), user_data);
	g_signal_connect (priv->app_proxy, "source-removed", G_CALLBACK (source_removed), user_data);
}

/*
 * app_section_set_object_path:
 * @self: an #AppSection
 * @bus: a #GDBusConnection
 * @bus_name: the bus name of the application
 * @object_path: the object path on which the app exports its actions and menus
 *
 * Sets the D-Bus object path exported by an instance of the application
 * associated with @self.  Actions and menus exported on that path will be
 * shown in the section.
 */
void
app_section_set_object_path (AppSection *self,
			     GDBusConnection *bus,
			     const gchar *bus_name,
			     const gchar *object_path)
{
	AppSectionPrivate *priv = self->priv;

	g_object_freeze_notify (G_OBJECT (self));
	app_section_unset_object_path (self);

	priv->app_proxy_cancellable = g_cancellable_new ();
	indicator_messages_application_proxy_new (bus,
						  G_DBUS_PROXY_FLAGS_NONE,
						  bus_name,
						  object_path,
						  priv->app_proxy_cancellable,
						  app_proxy_created,
						  self);

	priv->draws_attention = FALSE;

	priv->name_watch_id = g_bus_watch_name_on_connection (bus, bus_name, 0,
							      NULL, application_vanished,
							      self, NULL);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USES_CHAT_STATUS]);
	g_object_thaw_notify (G_OBJECT (self));

	g_action_group_change_action_state (G_ACTION_GROUP (priv->static_shortcuts),
					    "launch", g_variant_new_boolean (TRUE));
}

/*
 * app_section_unset_object_path:
 * @self: an #AppSection
 *
 * Unsets the object path set with app_section_set_object_path().  The section
 * will return to only showing application name and static shortcuts in the
 * menu.
 */
void
app_section_unset_object_path (AppSection *self)
{
	AppSectionPrivate *priv = self->priv;

	if (priv->app_proxy_cancellable) {
		g_cancellable_cancel (priv->app_proxy_cancellable);
		g_clear_object (&priv->app_proxy_cancellable);
	}
	g_clear_object (&priv->app_proxy);

	if (priv->name_watch_id) {
		g_bus_unwatch_name (priv->name_watch_id);
		priv->name_watch_id = 0;
	}

	g_simple_action_group_clear (priv->source_actions);
	g_menu_clear (priv->source_menu);

	priv->draws_attention = FALSE;
	g_clear_pointer (&priv->chat_status, g_free);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USES_CHAT_STATUS]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHAT_STATUS]);

	g_action_group_change_action_state (G_ACTION_GROUP (priv->static_shortcuts),
					    "launch", g_variant_new_boolean (FALSE));
}

gboolean
app_section_get_uses_chat_status (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;

	return priv->uses_chat_status;
}

const gchar *
app_section_get_status (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;

	return priv->chat_status;
}

void
app_section_set_status (AppSection  *self,
			const gchar *status)
{
	AppSectionPrivate * priv = self->priv;

	g_free (priv->chat_status);
	priv->chat_status = g_strdup (status);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHAT_STATUS]);
}
