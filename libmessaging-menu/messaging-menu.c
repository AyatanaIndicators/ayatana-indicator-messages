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

#include <gio/gdesktopappinfo.h>

/**
 * SECTION:messaging-menu
 * @title: MessagingMenuApp
 * @short_description: An application section in the messaging menu
 * @include: messaging-menu.h
 *
 * A #MessagingMenuApp represents an application section in the
 * Messaging Menu.  An application section is tied to an installed
 * application through a desktop file id, which must be passed to
 * messaging_menu_app_new().
 *
 * To register the application with the Messaging Menu, call
 * messaging_menu_app_register().  This signifies that the application
 * should be present in the menu and be marked as "running".
 *
 * The first menu item in an application section represents the
 * application itself, using the name and icon found in the associated
 * desktop file.  Activating this item starts the application.
 *
 * Following the application item, the Messaging Menu inserts all
 * shortcut actions found in the desktop file.  Actions whose
 * <code>NotShowIn</code> keyword contains "Messaging Menu" or whose
 * <code>OnlyShowIn</code> keyword does not contain "Messaging Menu"
 * will not appear (the <ulink
 * url="http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-1.1.html#extra-actions">
 * desktop file specification</ulink> contains a detailed explanation of
 * shortcut actions.)  An application cannot add, remove, or change
 * these shortcut items while it is running.
 *
 * Next, an application section contains menu items for message sources.
 * What exactly constitutes a message source depends on the type of
 * application:  an email client's message sources are folders
 * containing new messages, while those of a chat program are persons
 * that have contacted the user.
 *
 * A message source is represented in the menu by a label and optionally
 * also an icon.  It can be associated with either a count, a time, or
 * an arbitrary string, which will appear on the right side of the menu
 * item.
 *
 * When the user activates a source, the source is immediately removed
 * from the menu and the "activate-source" signal is emitted.
 *
 * Applications should always expose all the message sources available.
 * However, the Messaging Menu might limit the amount of sources it
 * displays to the user.
 *
 * The Messaging Menu offers users a way to set their chat status
 * (available, away, busy, invisible, or offline) for multiple
 * applications at once.  Applications that appear in the Messaging Menu
 * can integrate with this by setting the
 * "X-MessagingMenu-UsesChatSection" key in their desktop file to True.
 * Use messaging_menu_app_set_status() to signify that the application's
 * chat status has changed.  When the user changes status through the
 * Messaging Menu, the ::status-changed signal will be emitted.
 *
 * If the application stops running without calling
 * messaging_menu_app_unregister(), it will be marked as "not running".
 * Its application and shortcut items stay in the menu, but all message
 * sources are removed.  If messaging_menu_app_unregister() is called,
 * the application section is removed completely.
 *
 * More information about the design and recommended usage of the
 * Messaging Menu is available at <ulink
 * url="https://wiki.ubuntu.com/MessagingMenu">https://wiki.ubuntu.com/MessagingMenu</ulink>.
 */

/**
 * MessagingMenuApp:
 *
 * #MessagingMenuApp is an opaque structure.
 */
struct _MessagingMenuApp
{
  GObject parent_instance;

  GDesktopAppInfo *appinfo;
  int registered;  /* -1 for unknown */
  MessagingMenuStatus status;
  gboolean status_set;
  GSimpleActionGroup *source_actions;
  GMenu *menu;
  GDBusConnection *bus;

  IndicatorMessagesService *messages_service;
  guint watch_id;
  guint action_export_id;
  guint menu_export_id;

  GCancellable *cancellable;
};

G_DEFINE_TYPE (MessagingMenuApp, messaging_menu_app, G_TYPE_OBJECT);

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

static gchar *
messaging_menu_app_get_dbus_object_path (MessagingMenuApp *app)
{
  gchar *path;

  if (!app->appinfo)
    return NULL;

  path = g_strconcat ("/com/canonical/indicator/messages/",
                      g_app_info_get_id (G_APP_INFO (app->appinfo)),
                      NULL);

  g_strcanon (path, "/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", '_');

  return path;
}

static void
export_menus_and_actions (GObject      *source,
                          GAsyncResult *res,
                          gpointer      user_data)
{
  MessagingMenuApp *app = user_data;
  GError *error = NULL;
  gchar *object_path;

  object_path = messaging_menu_app_get_dbus_object_path (app);
  if (!object_path)
    return;

  app->bus = g_bus_get_finish (res, &error);
  if (app->bus == NULL)
    {
      g_warning ("unable to connect to session bus: %s", error->message);
      g_error_free (error);
      return;
    }

  app->action_export_id = g_dbus_connection_export_action_group (app->bus,
                                                                 object_path,
                                                                 G_ACTION_GROUP (app->source_actions),
                                                                 &error);
  if (!app->action_export_id)
    {
      g_warning ("unable to export action group: %s", error->message);
      g_clear_error (&error);
    }

  app->menu_export_id = g_dbus_connection_export_menu_model (app->bus,
                                                             object_path,
                                                             G_MENU_MODEL (app->menu),
                                                             &error);
  if (!app->menu_export_id)
    {
      g_warning ("unable to export menu: %s", error->message);
      g_clear_error (&error);
    }

  g_free (object_path);
}

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

  g_bus_get (G_BUS_TYPE_SESSION,
             app->cancellable,
             export_menus_and_actions,
             app);
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

  if (app->bus)
    {
      if (app->action_export_id > 0)
        g_dbus_connection_unexport_action_group (app->bus, app->action_export_id);

      if (app->menu_export_id > 0)
        g_dbus_connection_unexport_menu_model (app->bus, app->menu_export_id);

      app->action_export_id = 0;
      app->menu_export_id = 0;
      g_object_unref (app->bus);
      app->bus = NULL;
    }

  if (app->watch_id > 0)
    {
      g_bus_unwatch_name (app->watch_id);
      app->watch_id = 0;
    }

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

  /**
   * MessagingMenuApp:desktop-id:
   *
   * The desktop id of the application associated with this application
   * section.  Must be given when the #MessagingMenuApp is created.
   */
  properties[PROP_DESKTOP_ID] = g_param_spec_string ("desktop-id",
                                                     "Desktop Id",
                                                     "The desktop id of the associated application",
                                                     NULL,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  /**
   * MessagingMenuApp::activate-source:
   * @mmapp: the #MessagingMenuApp
   * @source_id: the source id that was activated
   *
   * Emitted when the user has activated the message source with id
   * @source_id.  The source is immediately removed from the menu,
   * handlers of this signal do not need to call
   * messaging_menu_app_remove_source().
   */
  signals[ACTIVATE_SOURCE] = g_signal_new ("activate-source",
                                           MESSAGING_MENU_TYPE_APP,
                                           G_SIGNAL_RUN_FIRST |
                                           G_SIGNAL_DETAILED,
                                           0,
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, G_TYPE_STRING);

  /**
   * MessagingMenuApp::status-changed:
   * @mmapp: the #MessagingMenuApp
   * @status: a #MessagingMenuStatus
   *
   * Emitted when the chat status is changed through the messaging menu.
   *
   * Applications which are registered to use the chat status should
   * change their status to @status upon receiving this signal.  Call
   * messaging_menu_app_set_status() to acknowledge that the application
   * changed its status.
   */
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
  if (app->status_set)
    messaging_menu_app_set_status (app, app->status);
}

static void
indicator_messages_appeared (GDBusConnection *bus,
                             const gchar     *name,
                             const gchar     *name_owner,
                             gpointer         user_data)
{
  MessagingMenuApp *app = user_data;

  indicator_messages_service_proxy_new (bus,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        "com.canonical.indicator.messages",
                                        "/com/canonical/indicator/messages/service",
                                        app->cancellable,
                                        created_messages_service,
                                        app);
}

static void
indicator_messages_vanished (GDBusConnection *bus,
                             const gchar     *name,
                             gpointer         user_data)
{
  MessagingMenuApp *app = user_data;

  if (app->messages_service)
    {
      g_signal_handlers_disconnect_by_func (app->messages_service,
                                            global_status_changed,
                                            app);
      g_clear_object (&app->messages_service);
    }
}

static void
messaging_menu_app_init (MessagingMenuApp *app)
{
  app->registered = -1;
  app->status_set = FALSE;
  app->bus = NULL;

  app->action_export_id = 0;
  app->menu_export_id = 0;

  app->cancellable = g_cancellable_new ();

  app->source_actions = g_simple_action_group_new ();
  app->menu = g_menu_new ();

  app->cancellable = g_cancellable_new ();

  app->watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                    "com.canonical.indicator.messages",
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    indicator_messages_appeared,
                                    indicator_messages_vanished,
                                    app,
                                    NULL);
}

/**
 * messaging_menu_new:
 * @desktop_id: a desktop file id. See g_desktop_app_info_new()
 *
 * Creates a new #MessagingMenuApp for the application associated with
 * @desktop_id.
 *
 * The application will not show up (nor be marked as "running") in the
 * Messaging Menu before messaging_menu_app_register() has been called.
 *
 * Returns: (transfer full): a new #MessagingMenuApp
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
 * Registers @app with the Messaging Menu.
 *
 * If the application doesn't already have a section in the Messaging
 * Menu, one will be created for it.  The application will also be
 * marked as "running".
 *
 * The application will be marked as "not running" as soon as @app is
 * destroyed.  The application launcher as well as shortcut actions will
 * remain in the menu.  To completely remove the application section
 * from the Messaging Menu, call messaging_menu_app_unregister().
 */
void
messaging_menu_app_register (MessagingMenuApp *app)
{
  gchar *object_path;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));

  app->registered = TRUE;

  /* state will be synced right after connecting to the service */
  if (!app->messages_service)
    return;

  object_path = messaging_menu_app_get_dbus_object_path (app);
  if (!object_path)
    return;

  indicator_messages_service_call_register_application (app->messages_service,
                                                        g_app_info_get_id (G_APP_INFO (app->appinfo)),
                                                        object_path,
                                                        app->cancellable,
                                                        NULL, NULL);

  g_free (object_path);
}

/**
 * messaging_menu_app_unregister:
 * @app: a #MessagingMenuApp
 *
 * Completely removes the @app from the Messaging Menu.  If the
 * application's launcher and shortcut actions should remain in the
 * menu, destroying @app with g_object_unref() suffices.
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

  if (!app->appinfo)
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
 *
 * Notify the Messaging Menu that the chat status of @app has changed to
 * @status.
 *
 * Connect to the ::status-changed signal to receive notification about
 * the user changing their global chat status through the Messaging
 * Menu.
 *
 * This function does nothing for applications whose desktop file does
 * not include X-MessagingMenu-UsesChatSection.
 */
void
messaging_menu_app_set_status (MessagingMenuApp    *app,
                               MessagingMenuStatus  status)
{
  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (status >= MESSAGING_MENU_STATUS_AVAILABLE &&
                    status <= MESSAGING_MENU_STATUS_OFFLINE);

  app->status = status;
  app->status_set = TRUE;

  /* state will be synced right after connecting to the service */
  if (!app->messages_service)
    return;

  if (!app->appinfo)
    return;

  indicator_messages_service_call_set_status (app->messages_service,
                                              g_app_info_get_id (G_APP_INFO (app->appinfo)),
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

  g_signal_emit (app, signals[STATUS_CHANGED], 0, status);
}

static void
source_action_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
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
  GSimpleAction *action;
  GMenuItem *menuitem;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (id != NULL);

  if (g_simple_action_group_lookup (app->source_actions, id))
    {
      g_warning ("a source with id '%s' already exists", id);
      return;
    }

  action = g_simple_action_new_stateful (id, NULL, state);
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

static GSimpleAction *
messaging_menu_app_get_source_action (MessagingMenuApp *app,
                                      const gchar      *source_id)

{
  GAction *action;

  g_return_val_if_fail (MESSAGING_MENU_IS_APP (app), NULL);
  g_return_val_if_fail (source_id != NULL, NULL);

  action = g_simple_action_group_lookup (app->source_actions, source_id);
  if (action == NULL)
    g_warning ("a source with id '%s' doesn't exist", source_id);

  return G_SIMPLE_ACTION (action);
}

static void
messaging_menu_app_set_source_action (MessagingMenuApp *app,
                                      const gchar      *source_id,
                                      guint             count,
                                      gint64            time,
                                      const gchar      *string)
{
  GSimpleAction *action;
  GVariant *state;
  gboolean draws_attention;
  GVariant *new_state;

  action = messaging_menu_app_get_source_action (app, source_id);
  if (!action)
    return;

  state = g_action_get_state (G_ACTION (action));
  g_variant_get_child (state, 3, "b", &draws_attention);

  new_state = g_variant_new ("(uxsb)", count, time, string, draws_attention);
  g_simple_action_set_state (action, new_state);

  g_variant_unref (state);
}

static void
messaging_menu_app_set_draws_attention (MessagingMenuApp *app,
                                        const gchar      *source_id,
                                        gboolean          draws_attention)
{
  GSimpleAction *action;
  GVariant *state;
  guint count;
  gint64 time;
  const gchar *string;
  GVariant *new_state;

  action = messaging_menu_app_get_source_action (app, source_id);
  if (!action)
    return;

  state = g_action_get_state (G_ACTION (action));
  g_variant_get (state, "(ux&sb)", &count, &time, &string, NULL);

  new_state = g_variant_new ("(uxsb)", count, time, string, TRUE);
  g_simple_action_set_state (action, new_state);

  g_variant_unref (state);
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
 * @time: the time when the source was created, in microseconds
 *
 * Inserts a new message source into the section representing @app and
 * initializes it with @time.  Use messaging_menu_app_insert_source() to
 * insert a source with the current time.
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
 * @id: a unique identifier for the source to be added
 * @icon: (allow-none): the icon associated with the source
 * @label: a user-visible string best describing the source
 * @time: the time when the source was created, in microseconds
 *
 * Appends a new message source to the end of the section representing
 * @app and initializes it with @time.  Use
 * messaging_menu_app_append_source() to append a source with the
 * current time.
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
      return;

  n_items = g_menu_model_get_n_items (G_MENU_MODEL (app->menu));
  for (i = 0; i < n_items; i++)
    {
      gchar *action;

      if (g_menu_model_get_item_attribute (G_MENU_MODEL (app->menu), i,
                                           "action", "s", &action))
        {
          if (!g_strcmp0 (action, source_id))
            {
              g_menu_remove (app->menu, i);
              break;
            }

          g_free (action);
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

static GMenuItem *
g_menu_find_item_with_action (GMenu        *menu,
                              const gchar  *action,
                              gint         *out_pos)
{
  gint i;
  gint n_elements;
  GMenuItem *item = NULL;

  n_elements = g_menu_model_get_n_items (G_MENU_MODEL (menu));

  for (i = 0; i < n_elements && item == NULL; i++)
    {
      GVariant *attr;

      item = g_menu_item_new_from_model (G_MENU_MODEL (menu), i);
      attr = g_menu_item_get_attribute_value (item, G_MENU_ATTRIBUTE_ACTION, G_VARIANT_TYPE_STRING);

      if (!g_str_equal (action, g_variant_get_string (attr, NULL)))
        g_clear_object (&item);

      g_variant_unref (attr);
    }

  if (item && out_pos)
    *out_pos = i - 1;

  return item;
}

static void
g_menu_replace_item (GMenu     *menu,
                     gint       pos,
                     GMenuItem *item)
{
  g_menu_remove (menu, pos);
  g_menu_insert_item (menu, pos, item);
}

/**
 * messaging_menu_app_set_source_label:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @label: the new label for the source
 *
 * Changes the label of @source_id to @label.
 */
void
messaging_menu_app_set_source_label (MessagingMenuApp *app,
                                     const gchar      *source_id,
                                     const gchar      *label)
{
  gint pos;
  GMenuItem *item;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (source_id != NULL);
  g_return_if_fail (label != NULL);

  item = g_menu_find_item_with_action (app->menu, source_id, &pos);
  if (item == NULL)
    return;

  g_menu_item_set_attribute (item, G_MENU_ATTRIBUTE_LABEL, "s", label);
  g_menu_replace_item (app->menu, pos, item);

  g_object_unref (item);
}

/**
 * messaging_menu_app_set_source_icon:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @icon: (allow-none): the new icon for the source
 *
 * Changes the icon of @source_id to @icon.
 */
void
messaging_menu_app_set_source_icon (MessagingMenuApp *app,
                                    const gchar      *source_id,
                                    GIcon            *icon)
{
  gint pos;
  GMenuItem *item;

  g_return_if_fail (MESSAGING_MENU_IS_APP (app));
  g_return_if_fail (source_id != NULL);

  item = g_menu_find_item_with_action (app->menu, source_id, &pos);
  if (item == NULL)
    return;

  if (icon)
    {
      gchar *iconstr;

      iconstr = g_icon_to_string (icon);
      g_menu_item_set_attribute (item, "x-canonical-icon", "s", iconstr);

      g_free (iconstr);
    }
  else
    {
      g_menu_item_set_attribute_value (item, "x-canonical-icon", NULL);
    }

  g_menu_replace_item (app->menu, pos, item);

  g_object_unref (item);
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
  messaging_menu_app_set_source_action (app, source_id, count, 0, "");
}

/**
 * messaging_menu_app_set_source_time:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @time: the new time for the source, in microseconds
 *
 * Updates the time of @source_id to @time.
 */
void
messaging_menu_app_set_source_time (MessagingMenuApp *app,
                                    const gchar      *source_id,
                                    gint64            time)
{
  messaging_menu_app_set_source_action (app, source_id, 0, time, "");
}

/**
 * messaging_menu_app_set_source_string:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 * @str: the new string for the source
 *
 * Updates the string displayed next to @source_id to @str.
 */
void
messaging_menu_app_set_source_string (MessagingMenuApp *app,
                                      const gchar      *source_id,
                                      const gchar      *str)
{
  messaging_menu_app_set_source_action (app, source_id, 0, 0, str);
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
  messaging_menu_app_set_draws_attention (app, source_id, TRUE);
}

/**
 * messaging_menu_app_remove_attention:
 * @app: a #MessagingMenuApp
 * @source_id: a source id
 *
 * Stop indicating that @source_id needs attention.
 *
 * This function does not need to be called when the source is removed
 * with messaging_menu_app_remove_source() or the user has activated the
 * source.
 *
 * Use messaging_menu_app_draw_attention() to make @source_id draw attention
 * again.
 */
void
messaging_menu_app_remove_attention (MessagingMenuApp *app,
                                     const gchar      *source_id)
{
  messaging_menu_app_set_draws_attention (app, source_id, TRUE);
}
