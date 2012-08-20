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
#include "gmenuutils.h"

struct _AppSectionPrivate
{
	GDesktopAppInfo * appinfo;
	guint unreadcount;

	IndicatorDesktopShortcuts * ids;

	GMenu *menu;
	GSimpleActionGroup *static_shortcuts;

	GMenuModel *remote_menu;
	GActionGroup *actions;

	gboolean draws_attention;
	gboolean uses_chat_status;

	guint name_watch_id;
};

enum {
	PROP_0,
	PROP_APPINFO,
	PROP_ACTIONS,
	PROP_DRAWS_ATTENTION,
	PROP_USES_CHAT_STATUS,
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
static void launch_action_change_state (GSimpleAction *action,
					GVariant      *value,
					gpointer       user_data);
static void app_section_set_app_info (AppSection *self,
				      GDesktopAppInfo *appinfo);
static gboolean any_action_draws_attention	(GActionGroup *group,
						 const gchar *ignored_action);
static void	action_added			(GActionGroup *group,
						 const gchar *action_name,
						 gpointer user_data);
static void	action_state_changed		(GActionGroup *group,
						 const gchar *action_name,
						 GVariant *value,
						 gpointer user_data);
static void	action_removed			(GActionGroup *group,
						 const gchar *action_name,
						 gpointer user_data);
static gboolean	action_draws_attention		(GVariant *state);

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

	g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
app_section_init (AppSection *self)
{
	AppSectionPrivate *priv;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
						  APP_SECTION_TYPE,
						  AppSectionPrivate);
	priv = self->priv;

	priv->appinfo = NULL;
	priv->unreadcount = 0;

	priv->menu = g_menu_new ();
	priv->static_shortcuts = g_simple_action_group_new ();

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
	AppSectionPrivate * priv = self->priv;

	g_clear_object (&priv->menu);
	g_clear_object (&priv->static_shortcuts);

	if (priv->name_watch_id) {
		g_bus_unwatch_name (priv->name_watch_id);
		priv->name_watch_id = 0;
	}

	if (priv->actions) {
		g_object_disconnect (priv->actions,
				     "any_signal::action-added", action_added, self,
				     "any_signal::action-state-changed", action_state_changed, self,
				     "any_signal::action-removed", action_removed, self,
				     NULL);
		g_clear_object (&priv->actions);
	}

	g_clear_object (&priv->remote_menu);

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
	AppSectionPrivate * priv = mi->priv;

	g_return_if_fail(priv->ids != NULL);

	if (!indicator_desktop_shortcuts_nick_exec(priv->ids, nick)) {
		g_warning("Unable to execute nick '%s' for desktop file '%s'",
			  nick, g_desktop_app_info_get_filename (priv->appinfo));
	}
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
	if (error) {
		if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			g_warning ("could not read X-MessagingMenu-UsesChatSection: %s",
				   error->message);
		}
		g_error_free (error);
		goto out;
	}

	if (self->priv->uses_chat_status)
		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USES_CHAT_STATUS]);

out:
	g_key_file_free (keyfile);
	g_free (contents);
}

static void
app_section_set_app_info (AppSection *self,
			  GDesktopAppInfo *appinfo)
{
	AppSectionPrivate *priv = self->priv;
	GSimpleAction *launch;
	GFile *keyfile;
	GMenuItem *item;
	gchar *iconname;

	g_return_if_fail (priv->appinfo == NULL);

	if (appinfo == NULL) {
		g_warning ("appinfo must not be NULL");
		return;
	}

	priv->appinfo = g_object_ref (appinfo);

	launch = g_simple_action_new_stateful ("launch", NULL, g_variant_new_boolean (FALSE));
	g_signal_connect (launch, "activate", G_CALLBACK (activate_cb), self);
	g_signal_connect (launch, "change-state", G_CALLBACK (launch_action_change_state), self);
	g_simple_action_group_insert (priv->static_shortcuts, G_ACTION (launch));

	item = g_menu_item_new (g_app_info_get_name (G_APP_INFO (priv->appinfo)), "launch");
	g_menu_item_set_attribute (item, "x-canonical-type", "s", "ImAppMenuItem");
	iconname = g_icon_to_string (g_app_info_get_icon (G_APP_INFO (priv->appinfo)));
	g_menu_item_set_attribute (item, INDICATOR_MENU_ATTRIBUTE_ICON_NAME, "s", iconname);
	g_free (iconname);

	g_menu_append_item (priv->menu, item);
	g_object_unref (item);

	/* Start to build static shortcuts */
	priv->ids = indicator_desktop_shortcuts_new(g_desktop_app_info_get_filename (priv->appinfo), "Messaging Menu");
	const gchar ** nicks = indicator_desktop_shortcuts_get_nicks(priv->ids);
	gint i;
	for (i = 0; nicks[i] != NULL; i++) {
		gchar *name;
		GSimpleAction *action;

		name = indicator_desktop_shortcuts_nick_get_name(priv->ids, nicks[i]);

		action = g_simple_action_new (nicks[i], NULL);
		g_signal_connect(action, "activate", G_CALLBACK (nick_activate_cb), self);
		g_simple_action_group_insert (priv->static_shortcuts, G_ACTION (action));

		g_menu_append (priv->menu, name, nicks[i]);

		g_free(name);
	}

	keyfile = g_file_new_for_path (g_desktop_app_info_get_filename (priv->appinfo));
	g_file_load_contents_async (keyfile, NULL, keyfile_loaded, self);
	g_object_unref (keyfile);

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_APPINFO]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);

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
	AppSectionPrivate * priv = mi->priv;
	GError *error = NULL;

	if (!g_app_info_launch (G_APP_INFO (priv->appinfo), NULL, NULL, &error)) {
		g_warning("Unable to execute application for desktop file '%s'",
			  g_desktop_app_info_get_filename (priv->appinfo));
	}
}

static void
launch_action_change_state (GSimpleAction *action,
			    GVariant      *value,
			    gpointer       user_data)
{
	g_simple_action_set_state (action, value);
}

guint
app_section_get_count (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), 0);
	AppSectionPrivate * priv = self->priv;

	return priv->unreadcount;
}

const gchar *
app_section_get_name (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), NULL);
	AppSectionPrivate * priv = self->priv;

	if (priv->appinfo) {
		return g_app_info_get_name(G_APP_INFO(priv->appinfo));
	}
	return NULL;
}

const gchar *
app_section_get_desktop (AppSection * self)
{
	g_return_val_if_fail(IS_APP_SECTION(self), NULL);
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
	return priv->actions ? priv->actions : G_ACTION_GROUP (priv->static_shortcuts);
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
	AppSectionPrivate * priv = self->priv;
	gchar **action_names;
	gchar **it;

	if (priv->actions == NULL)
		return;

	action_names = g_action_group_list_actions (priv->actions);

	for (it = action_names; *it; it++) {
		GVariant *state;

		state = g_action_group_get_action_state (priv->actions, *it);
		if (!state)
			continue;

		/* clear draws-attention while preserving other state */
		if (action_draws_attention (state)) {
			guint32 count;
			gint64 time;
			const gchar *str;
			GVariant *new_state;

			g_variant_get (state, "(ux&sb)", &count, &time, &str, NULL);

			new_state = g_variant_new ("(uxsb)", count, time, str, FALSE);
			g_action_group_change_action_state (priv->actions, *it, new_state);
		}

		g_variant_unref (state);
	}

	g_strfreev (action_names);
}

static void
application_vanished (GDBusConnection *bus,
		      const gchar *name,
		      gpointer user_data)
{
	AppSection *self = user_data;

	app_section_unset_object_path (self);
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

	priv->actions = G_ACTION_GROUP (g_dbus_action_group_get (bus, bus_name, object_path));

	priv->draws_attention = any_action_draws_attention (priv->actions, NULL);
	g_object_connect (priv->actions,
			  "signal::action-added", action_added, self,
			  "signal::action-state-changed", action_state_changed, self,
			  "signal::action-removed", action_removed, self,
			  NULL);

	priv->remote_menu = G_MENU_MODEL (g_dbus_menu_model_get (bus, bus_name, object_path));

	g_menu_append_section (priv->menu, NULL, priv->remote_menu);

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

	if (priv->name_watch_id) {
		g_bus_unwatch_name (priv->name_watch_id);
		priv->name_watch_id = 0;
	}

	if (priv->actions) {
		g_object_disconnect (priv->actions,
				     "any_signal::action-added", action_added, self,
				     "any_signal::action-state-changed", action_state_changed, self,
				     "any_signal::action-removed", action_removed, self,
				     NULL);
		g_clear_object (&priv->actions);
	}

	if (priv->remote_menu) {
		/* the last menu item points is linked to the app's menumodel */
		gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (priv->menu));
		g_menu_remove (priv->menu, n_items -1);
		g_clear_object (&priv->remote_menu);
	}

	priv->draws_attention = FALSE;

	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USES_CHAT_STATUS]);

	g_action_group_change_action_state (G_ACTION_GROUP (priv->static_shortcuts),
					    "launch", g_variant_new_boolean (FALSE));
}

static gboolean
action_draws_attention (GVariant *state)
{
	gboolean attention;

	if (state && g_variant_is_of_type (state, G_VARIANT_TYPE ("(uxsb)")))
		g_variant_get_child (state, 3, "b", &attention);
	else
		attention = FALSE;

	return attention;
}

static gboolean
any_action_draws_attention (GActionGroup *group,
			    const gchar *ignored_action)
{
	gchar **actions;
	gchar **it;
	gboolean attention = FALSE;

	actions = g_action_group_list_actions (group);

	for (it = actions; *it && !attention; it++) {
		GVariant *state;

		if (ignored_action && g_str_equal (ignored_action, *it))
			continue;

		state = g_action_group_get_action_state (group, *it);
		if (state) {
			attention = action_draws_attention (state);
			g_variant_unref (state);
		}
	}

	g_strfreev (actions);
	return attention;
}

static void
action_added (GActionGroup *group,
	      const gchar *action_name,
	      gpointer user_data)
{
	AppSection *self = user_data;
	GVariant *state;

	state = g_action_group_get_action_state (group, action_name);
	if (state) {
		self->priv->draws_attention |= action_draws_attention (state);
		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
		g_variant_unref (state);
	}
}

static void
action_state_changed (GActionGroup *group,
		      const gchar *action_name,
		      GVariant *value,
		      gpointer user_data)
{
	AppSection *self = user_data;

	self->priv->draws_attention = any_action_draws_attention (group, NULL);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);
}

static void
action_removed (GActionGroup *group,
		const gchar *action_name,
		gpointer user_data)
{
	AppSection *self = user_data;
	GVariant *state;

	state = g_action_group_get_action_state (group, action_name);
	if (!state)
		return;

	self->priv->draws_attention = any_action_draws_attention (group, action_name);
	g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAWS_ATTENTION]);

	g_variant_unref (state);
}

gboolean
app_section_get_uses_chat_status (AppSection *self)
{
	AppSectionPrivate * priv = self->priv;

	/* chat status is only useful when the app is running */
	return priv->uses_chat_status && priv->actions;
}
