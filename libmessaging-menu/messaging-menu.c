/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Lars Uebernickel <lars.uebernickel@canonical.com>
 */

#include "messaging-menu.h"
#include "indicator-messages-service.h"
#include "gtupleaction.h"

#include <gio/gdesktopappinfo.h>

/**
 * SECTION:messagingmenuapp
 * @title: MessagingMenuApp
 * @short_description: An application section in the messaging menu
 */
struct _MessagingMenuApp
{
  GObject parent_instance;

  GDesktopAppInfo *appinfo;
  int registered;  /* -1 for unknown */
  MessagingMenuStatus status;
  GSimpleActionGroup *source_actions;
  GMenu *menu;

  IndicatorMessagesService *messages_service;

  GCancellable *cancellable;
};

G_DEFINE_TYPE (MessagingMenuApp, messaging_menu_app, G_TYPE_OBJECT);

enum
{
  INDEX_COUNT,
  INDEX_TIME,
  INDEX_STRING,
  INDEX_DRAWS_ATTENTION
};

enum {
  PROP_0,
  PROP_DESKTOP_ID,
  N_PROPERTIES
};

enum {
  ACTIVATE_SOURCE,
  STATUS_CHANGED,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[N_SIGNALS];

static const gchar *status_ids[] = { "available", "away", "busy", "invisible", "offline" };

static void global_status_changed (IndicatorMessagesService *service,
                                   const gchar *status_str,
                                   gpointer user_data);

static void
messaging_menu_app_set_desktop_id (MessagingMenuApp *app,
                                   const gchar      *desktop_id)
{
  g_return_if_fail (desktop_id != NULL);

  /* no need to clean up, it's construct only */
  app->appinfo = g_desktop_app_info_new (desktop_id);
  if (app->appinfo == NULL)
    {
      g_warning ("could not find the desktop file for '%s'",
                 desktop_id);
    }
}

static void
messaging_menu_app_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MessagingMenuApp *app = MESSAGING_MENU_APP (object);

  switch (prop_id)
    {
    case PROP_DESKTOP_ID:
      messaging_menu_app_set_desktop_id (app, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
messaging_menu_app_finalize (GObject *object)
{
  G_OBJECT_CLASS (messaging_menu_app_parent_class)->finalize (object);
}

static void
messaging_menu_app_dispose (GObject *object)
{
  MessagingMenuApp *app = MESSAGING_MENU_APP (object);

  if (app->cancellable)
    {
      g_cancellable_cancel (app->cancellable);
      g_object_unref (app->cancellable);
      app->cancellable = NULL;
    }

  if (app->messages_service)
    {
      g_signal_handlers_disconnect_by_func (app->messages_service,
                                            global_status_changed,
                                            app);
      g_clear_object (&app->messages_service);
    }

  g_clear_object (&app->appinfo);
  g_clear_object (&app->source_actions);
  g_clear_object (&app->menu);

  G_OBJECT_CLASS (messaging_menu_app_parent_class)->dispose (object);
}

static void
messaging_menu_app_class_init (MessagingMenuAppClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = messaging_menu_app_set_property;
  object_class->finalize = messaging_menu_app_finalize;
  object_class->dispose = messaging_menu_app_dispose;

  properties[PROP_DESKTOP_ID] = g_param_spec_string ("desktop-id",
                                                     "Desktop Id",
                                                     "The desktop id of the associated application",
                                                     NULL,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  signals[ACTIVATE_SOURCE] = g_signal_new ("activate-source",
                                           MESSAGING_MENU_TYPE_APP,
                                           G_SIGNAL_RUN_FIRST |
                                           G_SIGNAL_DETAILED,
                                           0,
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[STATUS_CHANGED] = g_signal_new ("status-changed",
                                          MESSAGING_MENU_TYPE_APP,
                                          G_SIGNAL_RUN_FIRST,
                                          0,
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
created_messages_service (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  MessagingMenuApp *app = user_data;
  GError *error = NULL;

  app->messages_service = indicator_messages_service_proxy_new_finish (result, &error);
  if (!app->messages_service)
    {
      g_warning ("unable to connect to the mesaging menu service: %s", error->message);
      g_error_free (error);
      return;
    }

  g_signal_connect (app->messages_service, "status-changed",
                    G_CALLBACK (global_status_changed), app);

  /* sync current status */
  if (app->registered == TRUE)
    messaging_menu_app_register (app);
  else if (app->registered == FALSE)
    messaging_menu_app_unregister (app);
  messaging_menu_app_set_status (app, app->status);
}

static void
got_session_bus (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  MessagingMenuApp *app = user_data;
  GDBusConnection *bus;
  GError *error = NULL;
  guint id;

  bus = g_bus_get_finish (res, &error);
  if (bus == NULL)
    {
      g_warning ("unable to connect to session bus: %s", error->message);
      g_error_free (error);
      return;
    }

  id = g_dbus_connection_export_action_group (bus,
                                              "/com/canonical/indicator/messages",
                                              G_ACTION_GROUP (app->source_actions),
                                              &error);
  if (!id)
    {
      g_warning ("unable to export action group: %s", error->message);
      g_error_free (error);
    }

  id = g_dbus_connection_export_menu_model (bus,
                                            "/com/canonical/indicator/messages",
                                            G_MENU_MODEL (app->menu),
                                            &error);
  if (!id)
    {
      g_warning ("unable to export menu: %s", error->message);
      g_error_free (error);
    }

  indicator_messages_service_proxy_new (bus,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        "com.canonical.indicator.messages",
                                        "/com/canonical/indicator/messages/service",
                                        app->cancellable,
                                        created_messages_service,
                                        app);

  g_object_unref (bus);
}

static void
messaging_menu_app_init (MessagingMenuApp *app)
{
  app->registered = -1;
  app->status = MESSAGING_MENU_STATUS_OFFLINE;

  app->cancellable = g_cancellable_new ();

  app->source_actions = g_simple_action_group_new ();
  app->menu = g_menu_new ();

  app->cancellable = g_cancellable_new ();


  g_bus_get (G_BUS_TYPE_SESSION,
             app->cancellable,
             got_session_bus,
             app);
}

/**
 * messaging_menu_new:
 * @desktop_id: a desktop file id. See g_desktop_app_info_new()
 *
 * Creates a new #MessagingMenuApp for the application associated with
 * @desktop_id.
 *
 * If the application is already registered with the messaging menu, it will be
 * marked as "running".  Otherwise, call messaging_menu_app_register().
 *
 * The messaging menu will return to marking the application as not running as
 * soon as the returned #MessagingMenuApp is destroyed.
 *
 * Returns: (transfer-full): a new #MessagingMenuApp
 */
MessagingMenuApp *
messaging_menu_app_new (const gchar *desktop_id)
{
  return g_object_new (MESSAGING_MENU_TYPE_APP,
                       "desktop-id", desktop_id,
                       NULL);
}

/**
 * messaging_menu_app_register:
 * @app: a #MessagingMenuApp
 *
 * Registers @app with the messaging menu.
 *
 * The messaging menu will add a section with an app launcher and the shortcuts
 * defined in its desktop file.
 *
 * The application will be marked as "running" as long as @app is alive or
 * messaging_menu_app_unregister() is called.
 */
void
messaging_menu_app_register (MessagingMenuApp *app)
{
  g_return_if_fail (MESSAGING_MENU_IS_APP (app));

  app->registered = TRUE;

  /* state will be synced right after connecting to the service */
  if (!app->messages_service)
    return;

  indicator_messages_service_call_register_application (app->messages_service,
                                                        g_app_info_get_id (G_APP_INFO (app->appinfo)),
                                                        "/com/canonical/indicator/messages",
                                                        app->cancellable,
                                                        NULL, NULL);
}

/**
 * messaging_menu_app_unregister:
 * @app: a #MessagingMenuApp
 *
 * Completely removes the application associated with @desktop_id from the
 * messaging menu.
 *
 * Note: @app will remain valid and usable after this call.
 */
void
messaging_menu_app_unregister (MessagingMenuApp *app)
{
  g_return_if_fail (MESSAGING_MENU_IS_APP (app));

  app->registered = FALSE;

  /* state will be synced right after connecting to the service */
  if (!app->messages_service)
    return;

  indicator_messages_service_call_unregister_application (app->messages_service,
                                                          g_app_info_get_id (G_APP_INFO (app->appinfo)),
                                                          app->cancellable,
                                                          NULL, NULL);
}

/**
 * messaging_menu_app_set_status:
 * @app: a #MessagingMenuApp
 * @status: a #MessagingMenuStatus
 */
void
messaging_menu_app_set_status (MessagingMenuApp    *app,
                               MessagingMenuStatus  status)
{
  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (status >= MESSAGING_MENU_STATUS_AVAILABLE &&
                    status <= MESSAGING_MENU_STATUS_OFFLINE);

  app->status = status;

  /* state will be synced right after connecting to the service */
  if (!app->messages_service)
    return;

  indicator_messages_service_call_set_status (app->messages_service,
                                              status_ids [status],
                                              app->cancellable,
                                              NULL, NULL);
}

static int
status_from_string (const gchar *s)
{
  int i;

  if (!s)
    return -1;

  for (i = 0; i <= MESSAGING_MENU_STATUS_OFFLINE; i++)
    {
      if (g_str_equal (s, status_ids[i]))
        return i;
    }

  return -1;
}

static void
global_status_changed (IndicatorMessagesService *service,
                       const gchar *status_str,
                       gpointer user_data)
{
  MessagingMenuApp *app = user_data;
  int status;

  status = status_from_string (status_str);
  g_return_if_fail (status >= 0);

  app->status = (MessagingMenuStatus)status;
  g_signal_emit (app, signals[STATUS_CHANGED], 0, app->status);
}

static void
source_action_activated (GTupleAction *action,
                         GVariant     *parameter,
                         gpointer      user_data)
{
  MessagingMenuApp *app = user_data;
  const gchar *name = g_action_get_name (G_ACTION (action));
  GQuark q = g_quark_from_string (name);

  messaging_menu_app_remove_source (app, name);

  g_signal_emit (app, signals[ACTIVATE_SOURCE], q, name);
}

static void
messaging_menu_app_insert_source_action (MessagingMenuApp *app,
                                         gint              position,
                                         const gchar      *id,
                                         GIcon            *icon,
                                         const gchar      *label,
                                         GVariant         *state)
{
  GTupleAction *action;
  GMenuItem *menuitem;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (id != NULL);

  if (g_simple_action_group_lookup (app->source_actions, id))
    {
      g_warning ("a source with id '%s' already exists", id);
      return;
    }

  action = g_tuple_action_new (id, state);
  g_signal_connect (action, "activate",
                    G_CALLBACK (source_action_activated), app);
  g_simple_action_group_insert (app->source_actions, G_ACTION (action));
  g_object_unref (action);

  menuitem = g_menu_item_new (label, id);
  g_menu_item_set_attribute (menuitem, "x-canonical-type", "s", "ImSourceMenuItem");
  if (icon)
    {
      gchar *iconstr = g_icon_to_string (icon);
      g_menu_item_set_attribute (menuitem, "x-canonical-icon", "s", iconstr);
      g_free (iconstr);
    }
  g_menu_insert_item (app->menu, position, menuitem);
  g_object_unref (menuitem);
}

static void
messaging_menu_app_set_source_action (MessagingMenuApp *app,
                                      const gchar      *source_id,
                                      gsize             index,
                                      GVariant         *child)
{
  GAction *action;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (source_id != NULL);

  action = g_simple_action_group_lookup (app->source_actions, source_id);
  if (action == NULL)
    {
      g_warning ("a source with id '%s' doesn't exist", source_id);
      return;
    }

  g_tuple_action_set_child (G_TUPLE_ACTION (action), index, child);
}

/**
 * messaging_menu_app_insert_source:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: the icon associated with the source
 * @label: a user-visible string best describing the source
 *
 * Inserts a new message source into the section representing @app.  Equivalent
 * to calling messaging_menu_app_insert_source_with_time() with the current
 * time.
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_insert_source (MessagingMenuApp *app,
                                  gint              position,
                                  const gchar      *id,
                                  GIcon            *icon,
                                  const gchar      *label)
{
  messaging_menu_app_insert_source_with_time (app, position, id, icon, label,
                                              g_get_real_time ());
}

/**
 * messaging_menu_app_append_source:
 * @app: a #MessagingMenuApp
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 *
 * Appends a new message source to the end of the section representing @app.
 * Equivalent to calling messaging_menu_app_append_source_with_time() with the
 * current time.
 *
 * It is an error to add a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_append_source (MessagingMenuApp *app,
                                  const gchar      *id,
                                  GIcon            *icon,
                                  const gchar      *label)
{
  messaging_menu_app_insert_source (app, -1, id, icon, label);
}

/**
 * messaging_menu_app_insert_source_with_count:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @count: the count for the source
 *
 * Inserts a new message source into the section representing @app and
 * initializes it with @count.
 *
 * To update the count, use messaging_menu_app_set_source_count().
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_insert_source_with_count (MessagingMenuApp *app,
                                             gint              position,
                                             const gchar      *id,
                                             GIcon            *icon,
                                             const gchar      *label,
                                             guint             count)
{
  messaging_menu_app_insert_source_action (app, position, id, icon, label,
                                           g_variant_new ("(uxsb)", count, 0, "", FALSE));
}

/**
 * messaging_menu_app_append_source_with_count:
 * @app: a #MessagingMenuApp
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @count: the count for the source
 *
 * Appends a new message source to the end of the section representing @app and
 * initializes it with @count.
 *
 * To update the count, use messaging_menu_app_set_source_count().
 *
 * It is an error to add a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void messaging_menu_app_append_source_with_count (MessagingMenuApp *app,
                                                  const gchar      *id,
                                                  GIcon            *icon,
                                                  const gchar      *label,
                                                  guint             count)
{
  messaging_menu_app_insert_source_with_count (app, -1, id, icon, label, count);
}

/**
 * messaging_menu_app_insert_source_with_time:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @time: the time when the source was created
 *
 * Inserts a new message source into the section representing @app and
 * initializes it with @time.
 *
 * To change the time, use messaging_menu_app_set_source_time().
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_insert_source_with_time (MessagingMenuApp *app,
                                            gint              position,
                                            const gchar      *id,
                                            GIcon            *icon,
                                            const gchar      *label,
                                            gint64            time)
{
  messaging_menu_app_insert_source_action (app, position, id, icon, label,
                                           g_variant_new ("(uxsb)", 0, time, "", FALSE));
}

/**
 * messaging_menu_app_append_source_with_time:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @time: the time when the source was created
 *
 * Appends a new message source to the end of the section representing @app and
 * initializes it with @time.
 *
 * To change the time, use messaging_menu_app_set_source_time().
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_append_source_with_time (MessagingMenuApp *app,
                                            const gchar      *id,
                                            GIcon            *icon,
                                            const gchar      *label,
                                            gint64            time)
{
  messaging_menu_app_insert_source_with_time (app, -1, id, icon, label, time);
}

/**
 * messaging_menu_app_insert_source_with_string:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @str: a string associated with the source
 *
 * Inserts a new message source into the section representing @app and
 * initializes it with @str.
 *
 * To update the string, use messaging_menu_app_set_source_string().
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_insert_source_with_string (MessagingMenuApp *app,
                                              gint              position,
                                              const gchar      *id,
                                              GIcon            *icon,
                                              const gchar      *label,
                                              const gchar      *str)
{
  messaging_menu_app_insert_source_action (app, position, id, icon, label,
                                           g_variant_new ("(uxsb)", 0, 0, str, FALSE));
}

/**
 * messaging_menu_app_append_source_with_string:
 * @app: a #MessagingMenuApp
 * @position: the position at which to insert the source
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @str: a string associated with the source
 *
 * Appends a new message source to the end of the section representing @app and
 * initializes it with @str.
 *
 * To update the string, use messaging_menu_app_set_source_string().
 *
 * It is an error to insert a source with an @id which already exists.  Use
 * messaging_menu_app_has_source() to find out whether there is such a source.
 */
void
messaging_menu_app_append_source_with_string (MessagingMenuApp *app,
                                              const gchar      *id,
                                              GIcon            *icon,
                                              const gchar      *label,
                                              const gchar      *str)
{
  messaging_menu_app_insert_source_with_string (app, -1, id, icon, label, str);
}

/**
 * messaging_menu_app_remove_source:
 * @app: a #MessagingMenuApp
 * @source_id: the id of the source to remove
 *
 * Removes the source corresponding to @source_id from the menu.
 */
void
messaging_menu_app_remove_source (MessagingMenuApp *app,
                                  const gchar      *source_id)
{
  int n_items;
  int i;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (source_id != NULL);

  if (g_simple_action_group_lookup (app->source_actions, source_id) == NULL)
    {
      g_warning ("%s: a source with id '%s' doesn't exist", G_STRFUNC, source_id);
      return;
    }

  n_items = g_menu_model_get_n_items (G_MENU_MODEL (app->menu));
  for (i = 0; i < n_items; i++)
    {
      const gchar *action = NULL;

      g_menu_model_get_item_attribute (G_MENU_MODEL (app->menu), i,
                                       "action", "&s", &action);
      if (!g_strcmp0 (action, source_id))
        {
          g_menu_remove (app->menu, i);
          break;
        }
    }

  g_simple_action_group_remove (app->source_actions, source_id);
}

/**
 * messaging_menu_app_has_source:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 *
 * Returns: TRUE if there is a source associated with @source_id
 */
gboolean
messaging_menu_app_has_source (MessagingMenuApp *app,
                               const gchar      *source_id)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_APP (app), FALSE);
  g_return_val_if_fail (source_id != NULL, FALSE);

  return g_simple_action_group_lookup (app->source_actions, source_id) != NULL;
}

/**
 * messaging_menu_app_set_source_count:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @count: the new count for the source
 *
 * Updates the count of @source_id to @count.
 */
void messaging_menu_app_set_source_count (MessagingMenuApp *app,
                                          const gchar      *source_id,
                                          guint             count)
{
  messaging_menu_app_set_source_action (app, source_id, INDEX_COUNT,
                                        g_variant_new_uint32 (count));
}

/**
 * messaging_menu_app_set_source_time:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @time: the new time for the source
 *
 * Updates the time of @source_id to @time.
 *
 * Note that the time is only displayed if the source does not also have a
 * count associated with it.
 */
void
messaging_menu_app_set_source_time (MessagingMenuApp *app,
                                    const gchar      *source_id,
                                    gint64            time)
{
  messaging_menu_app_set_source_action (app, source_id, INDEX_TIME,
                                        g_variant_new_int64 (time));
}

/**
 * messaging_menu_app_set_source_string:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @string: the new string for the source
 *
 * Updates the string displayed next to @source_id to @str.
 *
 * Note that the string is only displayed if the source does not also have a
 * count or time associated with it.
 */
void
messaging_menu_app_set_source_string (MessagingMenuApp *app,
                                      const gchar      *source_id,
                                      const gchar      *str)
{
  messaging_menu_app_set_source_action (app, source_id, INDEX_STRING,
                                        g_variant_new_string (str));
}

/**
 * messaging_menu_app_draw_attention:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 *
 * Indicates that @source_id has important unread messages.  Currently, this
 * means that the messaging menu's envelope icon will turn blue.
 *
 * Use messaging_menu_app_remove_attention() to stop indicating that the source
 * needs attention.
 */
void
messaging_menu_app_draw_attention (MessagingMenuApp *app,
                                   const gchar      *source_id)
{
  messaging_menu_app_set_source_action (app, source_id, INDEX_DRAWS_ATTENTION,
                                        g_variant_new_boolean (TRUE));
}

/**
 * messaging_menu_app_remove_attention:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 *
 * Stop indicating that @source_id needs attention.
 *
 * Use messaging_menu_app_draw_attention() to make @source_id draw attention
 * again.
 */
void
messaging_menu_app_remove_attention (MessagingMenuApp *app,
                                     const gchar      *source_id)
{
  messaging_menu_app_set_source_action (app, source_id, INDEX_DRAWS_ATTENTION,
                                        g_variant_new_boolean (FALSE));
}
