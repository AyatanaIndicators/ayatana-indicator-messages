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

#include "ido-menu-item.h"

struct _IdoMenuItemPrivate
{
  GActionGroup *action_group;
  gchar *action;
  GVariant *target;

  GtkWidget *icon;
  GtkWidget *label;

  gboolean has_indicator;
  gboolean in_set_active;
};

enum
{
  PROP_0,
  PROP_MENU_ITEM,
  PROP_ACTION_GROUP,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (IdoMenuItem, ido_menu_item, GTK_TYPE_CHECK_MENU_ITEM);

static void
ido_menu_item_constructed (GObject *object)
{
  IdoMenuItemPrivate *priv = IDO_MENU_ITEM (object)->priv;
  GtkWidget *grid;

  priv->icon = g_object_ref (gtk_image_new ());
  gtk_widget_set_margin_right (priv->icon, 6);

  priv->label = g_object_ref (gtk_label_new (""));

  grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), priv->icon, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->label, 1, 0, 1, 1);

  gtk_container_add (GTK_CONTAINER (object), grid);
  gtk_widget_show_all (grid);

  G_OBJECT_CLASS (ido_menu_item_parent_class)->constructed (object);
}

static void
ido_menu_item_set_active (IdoMenuItem *self,
                          gboolean     active)
{
  /* HACK gtk_check_menu_item_set_active calls gtk_menu_item_activate.
   * Make sure our activate handler doesn't toggle the action as a
   * result of calling this function. */

  self->priv->in_set_active = TRUE;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self), active);
  self->priv->in_set_active = FALSE;
}

static void
ido_menu_item_set_has_indicator (IdoMenuItem *self,
                                 gboolean     has_indicator)
{
  if (has_indicator == self->priv->has_indicator)
    return;

  self->priv->has_indicator = has_indicator;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
ido_menu_item_set_state (IdoMenuItem *self,
                         GVariant    *state)
{
  IdoMenuItemPrivate *priv = self->priv;

  if (priv->target)
    {
      ido_menu_item_set_has_indicator (self, TRUE);
      ido_menu_item_set_active (self, FALSE);
      gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (self), TRUE);
      gtk_check_menu_item_set_inconsistent (GTK_CHECK_MENU_ITEM (self), FALSE);

      if (g_variant_is_of_type (state, G_VARIANT_TYPE_STRING))
        {
          ido_menu_item_set_active (self, g_variant_equal (priv->target, state));
        }
      else if (g_variant_is_of_type (state, G_VARIANT_TYPE ("as")) &&
               g_variant_is_of_type (priv->target, G_VARIANT_TYPE_STRING))
        {
          const gchar *target_str;
          const gchar **state_strs;
          const gchar **it;

          target_str = g_variant_get_string (priv->target, NULL);
          state_strs = g_variant_get_strv (state, NULL);

          it = state_strs;
          while (*it != NULL && !g_str_equal (*it, target_str))
            it++;

          if (*it != NULL)
            {
              ido_menu_item_set_active (self, TRUE);
              gtk_check_menu_item_set_inconsistent (GTK_CHECK_MENU_ITEM (self),
                                                    g_strv_length ((gchar **)state_strs) > 1);
            }

          g_free (state_strs);
        }
    }
  else if (g_variant_is_of_type (state, G_VARIANT_TYPE_BOOLEAN))
    {
      ido_menu_item_set_has_indicator (self, TRUE);
      gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (self), FALSE);
      ido_menu_item_set_active (self, g_variant_get_boolean (state));
    }
  else
    {
      ido_menu_item_set_has_indicator (self, FALSE);
    }
}

static void
ido_menu_item_set_action_name (IdoMenuItem *self,
                               const gchar *action_name)
{
  IdoMenuItemPrivate *priv = self->priv;
  gboolean enabled = FALSE;
  GVariant *state;
  const GVariantType *param_type;

  if (priv->action != NULL)
    g_free (priv->action);

  priv->action = g_strdup (action_name);

  if (priv->action_group != NULL && priv->action != NULL &&
      g_action_group_query_action (priv->action_group, priv->action,
                                   &enabled, &param_type, NULL, NULL, &state))
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);

      if (state)
        {
          ido_menu_item_set_state (self, state);
          g_variant_unref (state);
        }
    }
  else
    {
      ido_menu_item_set_active (self, FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
      ido_menu_item_set_has_indicator (self, FALSE);
    }
}

static void
ido_menu_item_action_added (GActionGroup *action_group,
                            gchar        *action_name,
                            gpointer      user_data)
{
  IdoMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    ido_menu_item_set_action_name (self, action_name);
}

static void
ido_menu_item_action_removed (GActionGroup *action_group,
                              gchar        *action_name,
                              gpointer      user_data)
{
  IdoMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
    }
}

static void
ido_menu_item_action_enabled_changed (GActionGroup *action_group,
                                      gchar        *action_name,
                                      gboolean      enabled,
                                      gpointer      user_data)
{
  IdoMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    gtk_widget_set_sensitive (GTK_WIDGET (self), enabled);
}

static void
ido_menu_item_action_state_changed (GActionGroup *action_group,
                                    gchar        *action_name,
                                    GVariant     *value,
                                    gpointer      user_data)
{
  IdoMenuItem *self = user_data;

  if (g_strcmp0 (self->priv->action, action_name) == 0)
    ido_menu_item_set_state (self, value);
}

static void
ido_menu_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  IdoMenuItem *self = IDO_MENU_ITEM (object);

  switch (property_id)
    {
    case PROP_MENU_ITEM:
      ido_menu_item_set_menu_item (self, G_MENU_ITEM (g_value_get_object (value)));
      break;

    case PROP_ACTION_GROUP:
      ido_menu_item_set_action_group (self, G_ACTION_GROUP (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ido_menu_item_dispose (GObject *object)
{
  IdoMenuItem *self = IDO_MENU_ITEM (object);

  if (self->priv->action_group)
      ido_menu_item_set_action_group (self, NULL);

  g_clear_object (&self->priv->icon);
  g_clear_object (&self->priv->label);

  if (self->priv->target)
    {
      g_variant_unref (self->priv->target);
      self->priv->target = NULL;
    }

  G_OBJECT_CLASS (ido_menu_item_parent_class)->dispose (object);
}

static void
ido_menu_item_finalize (GObject *object)
{
  IdoMenuItemPrivate *priv = IDO_MENU_ITEM (object)->priv;

  g_free (priv->action);

  G_OBJECT_CLASS (ido_menu_item_parent_class)->finalize (object);
}

static void
ido_menu_item_activate (GtkMenuItem *item)
{
  IdoMenuItemPrivate *priv = IDO_MENU_ITEM (item)->priv;
  GVariant *parameter;

  /* see ido_menu_item_set_active */
  if (!priv->in_set_active && priv->action && priv->action_group)
    {
      guint32 event_time = gtk_get_current_event_time ();

      if (priv->target)
        {
          parameter = priv->target;
        }
      else
        {
          parameter = g_variant_new_uint32 (event_time);
        }

      g_action_group_activate_action (priv->action_group, priv->action, parameter);
    }

  if (priv->in_set_active)
    GTK_MENU_ITEM_CLASS (ido_menu_item_parent_class)->activate (item);
}

static void
ido_menu_item_draw_indicator (GtkCheckMenuItem *item,
                              cairo_t          *cr)
{
  IdoMenuItem *self = IDO_MENU_ITEM (item);

  if (self->priv->has_indicator)
    GTK_CHECK_MENU_ITEM_CLASS (ido_menu_item_parent_class)
      ->draw_indicator (item, cr);
}

static void
ido_menu_item_class_init (IdoMenuItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkMenuItemClass *menu_item_class = GTK_MENU_ITEM_CLASS (klass);
  GtkCheckMenuItemClass *check_class = GTK_CHECK_MENU_ITEM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (IdoMenuItemPrivate));

  object_class->constructed = ido_menu_item_constructed;
  object_class->set_property = ido_menu_set_property;
  object_class->dispose = ido_menu_item_dispose;
  object_class->finalize = ido_menu_item_finalize;

  menu_item_class->activate = ido_menu_item_activate;

  check_class->draw_indicator = ido_menu_item_draw_indicator;

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
ido_menu_item_init (IdoMenuItem *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            IDO_TYPE_MENU_ITEM,
                                            IdoMenuItemPrivate);
}

void
ido_menu_item_set_menu_item (IdoMenuItem *self,
                             GMenuItem   *menuitem)
{
  gchar *iconstr = NULL;
  GIcon *icon = NULL;
  gchar *label = NULL;
  gchar *action = NULL;

  if (g_menu_item_get_attribute (menuitem, "x-canonical-icon", "s", &iconstr))
    {
      GError *error;

      /* only indent the label if icon is set to "" */
      if (iconstr[0] == '\0')
        {
          gint width;

          gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, NULL);
          gtk_widget_set_size_request (self->priv->icon, width, -1);
        }
      else
        {
          icon = g_icon_new_for_string (iconstr, &error);
          if (icon == NULL)
            {
              g_warning ("unable to set icon: %s", error->message);
              g_error_free (error);
            }
        }
      g_free (iconstr);
    }
  gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon), icon, GTK_ICON_SIZE_MENU);

  g_menu_item_get_attribute (menuitem, "label", "s", &label);
  gtk_label_set_label (GTK_LABEL (self->priv->label), label ? label : "");

  self->priv->target = g_menu_item_get_attribute_value (menuitem, "target", NULL);

  g_menu_item_get_attribute (menuitem, "action", "s", &action);
  ido_menu_item_set_action_name (self, action);

  if (icon)
    g_object_unref (icon);
  g_free (label);
  g_free (action);
}

void
ido_menu_item_set_action_group (IdoMenuItem  *self,
                                GActionGroup *action_group)
{
  IdoMenuItemPrivate *priv = self->priv;

  if (priv->action_group != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->action_group, ido_menu_item_action_added, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, ido_menu_item_action_removed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, ido_menu_item_action_enabled_changed, self);
      g_signal_handlers_disconnect_by_func (priv->action_group, ido_menu_item_action_state_changed, self);

      g_clear_object (&priv->action_group);
    }

  if (action_group != NULL)
    {
      priv->action_group = g_object_ref (action_group);

      g_signal_connect (priv->action_group, "action-added",
                        G_CALLBACK (ido_menu_item_action_added), self);
      g_signal_connect (priv->action_group, "action-removed",
                        G_CALLBACK (ido_menu_item_action_removed), self);
      g_signal_connect (priv->action_group, "action-enabled-changed",
                        G_CALLBACK (ido_menu_item_action_enabled_changed), self);
      g_signal_connect (priv->action_group, "action-state-changed",
                        G_CALLBACK (ido_menu_item_action_state_changed), self);
    }
}
