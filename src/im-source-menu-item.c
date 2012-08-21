/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
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

#include "im-source-menu-item.h"

#include <libintl.h>

struct _ImSourceMenuItemPrivate
{
  GActionGroup *action_group;
  gchar *action;

  GtkWidget *label;
  GtkWidget *detail;
};

enum
{
  PROP_0,
  PROP_MENU_ITEM,
  PROP_ACTION_GROUP,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (ImSourceMenuItem, im_source_menu_item, GTK_TYPE_MENU_ITEM);

static void
im_source_menu_item_constructed (GObject *object)
{
  ImSourceMenuItemPrivate *priv = IM_SOURCE_MENU_ITEM (object)->priv;
  GtkWidget *grid;
  gint icon_width;

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &icon_width, NULL);

  priv->label = g_object_ref (gtk_label_new (""));
  gtk_widget_set_margin_left (priv->label, icon_width + 2);

  priv->detail = g_object_ref (gtk_label_new (""));
  gtk_widget_set_halign (priv->detail, GTK_ALIGN_END);
  gtk_widget_set_hexpand (priv->detail, TRUE);
  gtk_misc_set_alignment (GTK_MISC (priv->label), 1.0, 0.5);
  gtk_style_context_add_class (gtk_widget_get_style_context (priv->detail), "accelerator");

  grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), priv->label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->detail, 1, 0, 1, 1);

  gtk_container_add (GTK_CONTAINER (object), grid);
  gtk_widget_show_all (grid);

  G_OBJECT_CLASS (im_source_menu_item_parent_class)->constructed (object);
}

/* collapse_whitespace:
 * @str: the source string
 *
 * Collapses all occurences of consecutive whitespace charactes in @str
 * into a single space.
 *
 * Returns: (transfer full): a newly-allocated string
 */
static gchar *
collapse_whitespace (const gchar *str)
{
  GString *result;
  gboolean in_space = FALSE;

  if (str == NULL)
    return NULL;

  result = g_string_new ("");

  while (*str)
    {
      gunichar c = g_utf8_get_char_validated (str, -1);

      if (c < 0)
        break;

      if (!g_unichar_isspace (c))
        {
          g_string_append_unichar (result, c);
          in_space = FALSE;
        }
      else if (!in_space)
        {
          g_string_append_c (result, ' ');
          in_space = TRUE;
        }

      str = g_utf8_next_char (str);
    }

  return g_string_free (result, FALSE);
}

static gchar *
im_source_menu_item_time_span_string (gint64 timestamp)
{
  gchar *str;
  gint64 span;
  gint hours;
  gint minutes;

  span = MAX (g_get_real_time () - timestamp, 0) / G_USEC_PER_SEC;
  hours = span / 3600;
  minutes = (span / 60) % 60;

  if (hours == 0)
    {
      /* TRANSLATORS: number of minutes that have passed */
      str = g_strdup_printf (ngettext ("%d m", "%d m", minutes), minutes);
    }
  else
    {
      /* TRANSLATORS: number of hours that have passed */
      str = g_strdup_printf (ngettext ("%d h", "%d h", hours), hours);
    }

  return str;
}

static gboolean
im_source_menu_item_set_state (ImSourceMenuItem *self,
                               GVariant         *state)
{
  ImSourceMenuItemPrivate *priv = self->priv;
  guint32 count;
  gint64 time;
  const gchar *str;
  gchar *detail;

  g_return_val_if_fail (g_variant_is_of_type (state, G_VARIANT_TYPE ("(uxsb)")), FALSE);

  g_variant_get (state, "(ux&sb)", &count, &time, &str, NULL);

  if (count != 0)
    detail = g_strdup_printf ("%d", count);
  else if (time != 0)
    detail = im_source_menu_item_time_span_string (time);
  else if (str != NULL && *str)
    detail = collapse_whitespace (str);
  else
    detail = NULL;

  gtk_label_set_text (GTK_LABEL (priv->detail), detail ? detail : "");

  g_free (detail);
  return TRUE;
}

static void
im_source_menu_item_set_action_name (ImSourceMenuItem *self,
                                     const gchar      *action_name)
{
  ImSourceMenuItemPrivate *priv = self->priv;
  gboolean enabled = FALSE;
  GVariant *state;

  if (priv->action != NULL)
    g_free (priv->action);

  priv->action = g_strdup (action_name);

  if (priv->action_group != NULL && priv->action != NULL &&
      g_action_group_query_action (priv->action_group, priv->action,
                                   &enabled, NULL, NULL, NULL, &state))
    {
      if (!state || !im_source_menu_item_set_state (self, state))
        enabled = FALSE;

      if (state)
        g_variant_unref (state);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);
}

static void
im_source_menu_item_action_added (GActionGroup *action_group,
                                  gchar        *action_name,
                                  gpointer      user_data)
{
  ImSourceMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    im_source_menu_item_set_action_name (self, action_name);
}

static void
im_source_menu_item_action_removed (GActionGroup *action_group,
                                    gchar        *action_name,
                                    gpointer      user_data)
{
  ImSourceMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
    }
}

static void
im_source_menu_item_action_enabled_changed (GActionGroup *action_group,
                                            gchar        *action_name,
                                            gboolean      enabled,
                                            gpointer      user_data)
{
  ImSourceMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);
}

static void
im_source_menu_item_action_state_changed (GActionGroup *action_group,
                                          gchar        *action_name,
                                          GVariant     *value,
                                          gpointer      user_data)
{
  ImSourceMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    im_source_menu_item_set_state (self, value);
}

static void
im_source_menu_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ImSourceMenuItem *self = IM_SOURCE_MENU_ITEM (object);

  switch (property_id)
    {
    case PROP_MENU_ITEM:
      im_source_menu_item_set_menu_item (self, G_MENU_ITEM (g_value_get_object (value)));
      break;

    case PROP_ACTION_GROUP:
      im_source_menu_item_set_action_group (self, G_ACTION_GROUP (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
im_source_menu_item_dispose (GObject *object)
{
  ImSourceMenuItem *self = IM_SOURCE_MENU_ITEM (object);

  if (self->priv->action_group)
      im_source_menu_item_set_action_group (self, NULL);

  g_clear_object (&self->priv->label);
  g_clear_object (&self->priv->detail);

  G_OBJECT_CLASS (im_source_menu_item_parent_class)->dispose (object);
}

static void
im_source_menu_item_finalize (GObject *object)
{
  ImSourceMenuItemPrivate *priv = IM_SOURCE_MENU_ITEM (object)->priv;

  g_free (priv->action);

  G_OBJECT_CLASS (im_source_menu_item_parent_class)->finalize (object);
}

static void
im_source_menu_item_activate (GtkMenuItem *item)
{
  ImSourceMenuItemPrivate *priv = IM_SOURCE_MENU_ITEM (item)->priv;

  if (priv->action && priv->action_group)
    g_action_group_activate_action (priv->action_group, priv->action, NULL);
}

static void
im_source_menu_item_class_init (ImSourceMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkMenuItemClass *menu_item_class = GTK_MENU_ITEM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ImSourceMenuItemPrivate));

  object_class->constructed = im_source_menu_item_constructed;
  object_class->set_property = im_source_menu_set_property;
  object_class->dispose = im_source_menu_item_dispose;
  object_class->finalize = im_source_menu_item_finalize;

  menu_item_class->activate = im_source_menu_item_activate;

  properties[PROP_MENU_ITEM] = g_param_spec_object ("menu-item",
                                                    "Menu item",
                                                    "The model GMenuItem for this menu item",
                                                    G_TYPE_MENU_ITEM,
                                                    G_PARAM_WRITABLE |
                                                    G_PARAM_STATIC_STRINGS);

  properties[PROP_ACTION_GROUP] = g_param_spec_object ("action-group",
                                                       "Action group",
                                                       "The action group associated with this menu item",
                                                       G_TYPE_ACTION_GROUP,
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
im_source_menu_item_init (ImSourceMenuItem *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            IM_TYPE_SOURCE_MENU_ITEM,
                                            ImSourceMenuItemPrivate);
}

void
im_source_menu_item_set_menu_item (ImSourceMenuItem *self,
                                   GMenuItem        *menuitem)
{
  gchar *label;
  gchar *action = NULL;

  g_menu_item_get_attribute (menuitem, "label", "s", &label);
  gtk_label_set_label (GTK_LABEL (self->priv->label), label ? label : "");

  g_menu_item_get_attribute (menuitem, "action", "s", &action);
  im_source_menu_item_set_action_name (self, action);

  g_free (label);
  g_free (action);
}

void
im_source_menu_item_set_action_group (ImSourceMenuItem *self,
                                      GActionGroup     *action_group)
{
  ImSourceMenuItemPrivate *priv = self->priv;

  if (priv->action_group != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->action_group, im_source_menu_item_action_added, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_source_menu_item_action_removed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_source_menu_item_action_enabled_changed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_source_menu_item_action_state_changed, self);

      g_clear_object (&priv->action_group);
    }

  if (action_group != NULL)
    {
      priv->action_group = g_object_ref (action_group);

      g_signal_connect (priv->action_group, "action-added",
                        G_CALLBACK (im_source_menu_item_action_added), self);
      g_signal_connect (priv->action_group, "action-removed",
                        G_CALLBACK (im_source_menu_item_action_removed), self);
      g_signal_connect (priv->action_group, "action-enabled-changed",
                        G_CALLBACK (im_source_menu_item_action_enabled_changed), self);
      g_signal_connect (priv->action_group, "action-state-changed",
                        G_CALLBACK (im_source_menu_item_action_state_changed), self);
    }
}
