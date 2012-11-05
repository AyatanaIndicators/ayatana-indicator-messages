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

#include "messaging-menu-message.h"

typedef GObjectClass MessagingMenuMessageClass;

struct _MessagingMenuMessage
{
  GObject parent;

  gchar *id;
  GIcon *icon;
  gchar *title;
  gchar *subtitle;
  gchar *body;
  gint64 time;
  gboolean draws_attention;
};

G_DEFINE_TYPE (MessagingMenuMessage, messaging_menu_message, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_ID,
  PROP_ICON,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_BODY,
  PROP_TIME,
  PROP_DRAWS_ATTENTION,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

static void
messaging_menu_message_dispose (GObject *object)
{
  MessagingMenuMessage *msg = MESSAGING_MENU_MESSAGE (object);

  g_clear_object (&msg->icon);

  G_OBJECT_CLASS (messaging_menu_message_parent_class)->dispose (object);
}

static void
messaging_menu_message_finalize (GObject *object)
{
  MessagingMenuMessage *msg = MESSAGING_MENU_MESSAGE (object);

  g_free (msg->id);
  g_free (msg->title);
  g_free (msg->subtitle);
  g_free (msg->body);

  G_OBJECT_CLASS (messaging_menu_message_parent_class)->finalize (object);
}

static void
messaging_menu_message_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MessagingMenuMessage *msg = MESSAGING_MENU_MESSAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_string (value, msg->id);
      break;

    case PROP_ICON:
      g_value_set_object (value, msg->icon);
      break;

    case PROP_TITLE:
      g_value_set_string (value, msg->title);
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, msg->subtitle);
      break;

    case PROP_BODY:
      g_value_set_string (value, msg->body);

    case PROP_TIME:
      g_value_set_int64 (value, msg->time);
      break;

    case PROP_DRAWS_ATTENTION:
      g_value_set_boolean (value, msg->draws_attention);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
messaging_menu_message_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MessagingMenuMessage *msg = MESSAGING_MENU_MESSAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      msg->id = g_value_dup_string (value);
      break;

    case PROP_ICON:
      msg->icon = g_value_dup_object (value);
      break;

    case PROP_TITLE:
      msg->title = g_value_dup_string (value);
      break;

    case PROP_SUBTITLE:
      msg->subtitle = g_value_dup_string (value);
      break;

    case PROP_BODY:
      msg->body = g_value_dup_string (value);

    case PROP_TIME:
      msg->time = g_value_get_int64 (value);
      break;

    case PROP_DRAWS_ATTENTION:
      messaging_menu_message_set_draws_attention (msg, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
messaging_menu_message_class_init (MessagingMenuMessageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = messaging_menu_message_dispose;
  object_class->finalize = messaging_menu_message_finalize;
  object_class->get_property = messaging_menu_message_get_property;
  object_class->set_property = messaging_menu_message_set_property;

  properties[PROP_ID] = g_param_spec_string ("id", "Id",
                                             "Unique id of the message",
                                             NULL,
                                             G_PARAM_CONSTRUCT_ONLY |
                                             G_PARAM_READWRITE |
                                             G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON] = g_param_spec_object ("icon", "Icon",
                                               "Icon of the message",
                                               G_TYPE_ICON,
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_READWRITE |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_TITLE] = g_param_spec_string ("title", "Title",
                                                "Title of the message",
                                                NULL,
                                                G_PARAM_CONSTRUCT_ONLY |
                                                G_PARAM_READWRITE |
                                                G_PARAM_STATIC_STRINGS);

  properties[PROP_SUBTITLE] = g_param_spec_string ("subtitle", "Subtitle",
                                                   "Subtitle of the message",
                                                   NULL,
                                                   G_PARAM_CONSTRUCT_ONLY |
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);

  properties[PROP_BODY] = g_param_spec_string ("body", "Body",
                                               "First lines of the body of the message",
                                               NULL,
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_READWRITE |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_TIME] = g_param_spec_int64 ("time", "Time",
                                              "Time the message was sent, in microseconds", 0, G_MAXINT64, 0,
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_READWRITE |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_DRAWS_ATTENTION] = g_param_spec_boolean ("draws-attention", "Draws attention",
                                                           "Whether the message should draw attention",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (klass, NUM_PROPERTIES, properties);
}

static void
messaging_menu_message_init (MessagingMenuMessage *self)
{
}

/**
 * messaging_menu_message_new:
 * @id: unique id of the message
 * @icon: (transfer full): a #GIcon representing the message
 * @title: the title of the message
 * @subtitle: (allow-none): the subtitle of the message
 * @body: (allow-none): the message body
 * @time: the time the message was received
 *
 * Creates a new #MessagingMenuMessage.
 *
 * Returns: (transfer full): a new #MessagingMenuMessage
 */
MessagingMenuMessage *
messaging_menu_message_new (const gchar *id,
                            GIcon       *icon,
                            const gchar *title,
                            const gchar *subtitle,
                            const gchar *body,
                            gint64       time)
{
  g_return_val_if_fail (id != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);

  return g_object_new (MESSAGING_MENU_TYPE_MESSAGE,
                       "id", id,
                       "icon", icon,
                       "title", title,
                       "subtitle", subtitle,
                       "body", body,
                       "time", time,
                       NULL);
}

/**
 * messaging_menu_message_get_id:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: the unique id of @msg
 */
const gchar *
messaging_menu_message_get_id (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), NULL);

  return msg->id;
}

/**
 * messaging_menu_message_get_icon:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: (transfer none): the icon of @msg
 */
GIcon *
messaging_menu_message_get_icon (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), NULL);

  return msg->icon;
}

/**
 * messaging_menu_message_get_title:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: the title of @msg
 */
const gchar *
messaging_menu_message_get_title (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), NULL);

  return msg->title;
}

/**
 * messaging_menu_message_get_subtitle:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: the subtitle of @msg
 */
const gchar *
messaging_menu_message_get_subtitle (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), NULL);

  return msg->subtitle;
}

/**
 * messaging_menu_message_get_body:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: the body of @msg
 */
const gchar *
messaging_menu_message_get_body (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), NULL);

  return msg->body;
}

/**
 * messaging_menu_message_get_time:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: the time at which @msg was received
 */
gint64
messaging_menu_message_get_time (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), 0);

  return msg->time;
}

/**
 * messaging_menu_message_get_draws_attention:
 * @msg: a #MessagingMenuMessage
 *
 * Returns: whether @msg is drawing attention
 */
gboolean
messaging_menu_message_get_draws_attention  (MessagingMenuMessage *msg)
{
  g_return_val_if_fail (MESSAGING_MENU_IS_MESSAGE (msg), FALSE);

  return msg->draws_attention;
}

/**
 * messaging_menu_message_set_draws_attention:
 * @msg: a #MessagingMenuMessage
 * @draws_attention: whether @msg should draw attention
 *
 * Sets whether @msg is drawing attention.
 */
void
messaging_menu_message_set_draws_attention  (MessagingMenuMessage *msg,
                                             gboolean              draws_attention)
{
  g_return_if_fail (MESSAGING_MENU_IS_MESSAGE (msg));

  msg->draws_attention = draws_attention;
  g_object_notify_by_pspec (G_OBJECT (msg), properties[PROP_DRAWS_ATTENTION]);
}
