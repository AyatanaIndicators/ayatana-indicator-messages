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

#include "im-app-menu-item.h"

struct _ImAppMenuItemPrivate
{
  GActionGroup *action_group;
  gchar *action;
  gboolean is_running;

  GtkWidget *icon;
  GtkWidget *label;
};

enum
{
  PROP_0,
  PROP_MENU_ITEM,
  PROP_ACTION_GROUP,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (ImAppMenuItem, im_app_menu_item, GTK_TYPE_MENU_ITEM);

static void
im_app_menu_item_constructed (GObject *object)
{
  ImAppMenuItemPrivate *priv = IM_APP_MENU_ITEM (object)->priv;
  GtkWidget *grid;

  priv->icon = g_object_ref (gtk_image_new ());

  priv->label = g_object_ref (gtk_label_new (""));

  grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), priv->icon, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->label, 1, 0, 1, 1);

  gtk_container_add (GTK_CONTAINER (object), grid);
  gtk_widget_show_all (grid);

  G_OBJECT_CLASS (im_app_menu_item_parent_class)->constructed (object);
}

static void
im_app_menu_item_set_action_name (ImAppMenuItem *self,
                                  const gchar   *action_name)
{
  ImAppMenuItemPrivate *priv = self->priv;
  gboolean enabled = FALSE;
  GVariant *state;

  if (priv->action != NULL)
    g_free (priv->action);

  priv->action = g_strdup (action_name);

  priv->is_running = FALSE;

  if (priv->action_group != NULL && priv->action != NULL &&
      g_action_group_query_action (priv->action_group, priv->action,
                                   &enabled, NULL, NULL, NULL, &state))
    {
      if (state && g_variant_is_of_type (state, G_VARIANT_TYPE ("b")))
        priv->is_running = g_variant_get_boolean (state);
      else
        enabled = FALSE;

      if (state)
        g_variant_unref (state);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
im_app_menu_item_action_added (GActionGroup *action_group,
                               gchar        *action_name,
                               gpointer      user_data)
{
  ImAppMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    im_app_menu_item_set_action_name (self, action_name);
}

static void
im_app_menu_item_action_removed (GActionGroup *action_group,
                                 gchar        *action_name,
                                 gpointer      user_data)
{
  ImAppMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
      self->priv->is_running = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

static void
im_app_menu_item_action_enabled_changed (GActionGroup *action_group,
                                         gchar        *action_name,
                                         gboolean      enabled,
                                         gpointer      user_data)
{
  ImAppMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);
}

static void
im_app_menu_item_action_state_changed (GActionGroup *action_group,
                                       gchar        *action_name,
                                       GVariant     *value,
                                       gpointer      user_data)
{
  ImAppMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    {
      g_return_if_fail (g_variant_is_of_type (value, G_VARIANT_TYPE ("b")));

      self->priv->is_running = g_variant_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

static void
im_app_menu_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ImAppMenuItem *self = IM_APP_MENU_ITEM (object);

  switch (property_id)
    {
    case PROP_MENU_ITEM:
      im_app_menu_item_set_menu_item (self, G_MENU_ITEM (g_value_get_object (value)));
      break;

    case PROP_ACTION_GROUP:
      im_app_menu_item_set_action_group (self, G_ACTION_GROUP (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
im_app_menu_item_dispose (GObject *object)
{
  ImAppMenuItem *self = IM_APP_MENU_ITEM (object);

  if (self->priv->action_group)
      im_app_menu_item_set_action_group (self, NULL);

  g_clear_object (&self->priv->icon);
  g_clear_object (&self->priv->label);

  G_OBJECT_CLASS (im_app_menu_item_parent_class)->dispose (object);
}

static void
im_app_menu_item_finalize (GObject *object)
{
  ImAppMenuItemPrivate *priv = IM_APP_MENU_ITEM (object)->priv;

  g_free (priv->action);

  G_OBJECT_CLASS (im_app_menu_item_parent_class)->finalize (object);
}

static gboolean
im_app_menu_item_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  ImAppMenuItemPrivate *priv = IM_APP_MENU_ITEM (widget)->priv;

  GTK_WIDGET_CLASS (im_app_menu_item_parent_class)->draw (widget, cr);

  if (priv->is_running)
    {
      const int arrow_width = 5;
      const double half_arrow_height = 4.5;
      GtkAllocation alloc;
      GdkRGBA color;
      double center;

      gtk_widget_get_allocation (widget, &alloc);

      gtk_style_context_get_color (gtk_widget_get_style_context (widget),
                                   gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);

      center = alloc.height / 2 + 0.5;

      cairo_move_to (cr, 0, center - half_arrow_height);
      cairo_line_to (cr, 0, center + half_arrow_height);
      cairo_line_to (cr, arrow_width, center);
      cairo_close_path (cr);

      cairo_fill (cr);
    }

  return FALSE;
}

static void
im_app_menu_item_activate (GtkMenuItem *item)
{
  ImAppMenuItemPrivate *priv = IM_APP_MENU_ITEM (item)->priv;

  if (priv->action && priv->action_group)
    g_action_group_activate_action (priv->action_group, priv->action, NULL);
}

static void
im_app_menu_item_class_init (ImAppMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkMenuItemClass *menu_item_class = GTK_MENU_ITEM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ImAppMenuItemPrivate));

  object_class->constructed = im_app_menu_item_constructed;
  object_class->set_property = im_app_menu_set_property;
  object_class->dispose = im_app_menu_item_dispose;
  object_class->finalize = im_app_menu_item_finalize;

  widget_class->draw = im_app_menu_item_draw;

  menu_item_class->activate = im_app_menu_item_activate;

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
im_app_menu_item_init (ImAppMenuItem *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            IM_TYPE_APP_MENU_ITEM,
                                            ImAppMenuItemPrivate);
}

void
im_app_menu_item_set_menu_item (ImAppMenuItem *self,
                                GMenuItem     *menuitem)
{
  gchar *iconstr = NULL;
  GIcon *icon = NULL;
  gchar *label;
  gchar *action = NULL;

  if (g_menu_item_get_attribute (menuitem, "x-canonical-icon", "s", &iconstr))
    {
      GError *error;

      icon = g_icon_new_for_string (iconstr, &error);
      if (icon == NULL)
        {
          g_warning ("unable to set icon: %s", error->message);
          g_error_free (error);
        }
      g_free (iconstr);
    }
  gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon), icon, GTK_ICON_SIZE_MENU);

  g_menu_item_get_attribute (menuitem, "label", "s", &label);
  gtk_label_set_label (GTK_LABEL (self->priv->label), label ? label : "");

  g_menu_item_get_attribute (menuitem, "action", "s", &action);
  im_app_menu_item_set_action_name (self, action);

  if (icon)
    g_object_unref (icon);
  g_free (label);
  g_free (action);
}

void
im_app_menu_item_set_action_group (ImAppMenuItem *self,
                                   GActionGroup  *action_group)
{
  ImAppMenuItemPrivate *priv = self->priv;

  if (priv->action_group != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->action_group, im_app_menu_item_action_added, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_app_menu_item_action_removed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_app_menu_item_action_enabled_changed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, im_app_menu_item_action_state_changed, self);

      g_clear_object (&priv->action_group);
    }

  if (action_group != NULL)
    {
      priv->action_group = g_object_ref (action_group);

      g_signal_connect (priv->action_group, "action-added",
                        G_CALLBACK (im_app_menu_item_action_added), self);
      g_signal_connect (priv->action_group, "action-removed",
                        G_CALLBACK (im_app_menu_item_action_removed), self);
      g_signal_connect (priv->action_group, "action-enabled-changed",
                        G_CALLBACK (im_app_menu_item_action_enabled_changed), self);
      g_signal_connect (priv->action_group, "action-state-changed",
                        G_CALLBACK (im_app_menu_item_action_state_changed), self);
    }
}
