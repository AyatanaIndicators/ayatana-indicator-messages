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

#include "im-phone-menu.h"

#include <string.h>

typedef GObjectClass ImPhoneMenuClass;

struct _ImPhoneMenu
{
  GObject parent;

  GMenu *toplevel_menu;
  GMenu *message_section;
  GMenu *source_section;

};

G_DEFINE_TYPE (ImPhoneMenu, im_phone_menu, G_TYPE_OBJECT);

typedef void (*ImMenuForeachFunc) (GMenuModel *menu, gint pos);

static void
im_phone_menu_foreach_item_with_action (GMenuModel        *menu,
                                        const gchar       *action,
                                        ImMenuForeachFunc  func)
{
  gint n_items;
  gint i;

  n_items = g_menu_model_get_n_items (menu);
  for (i = 0; i < n_items; i++)
    {
      gchar *item_action;

      g_menu_model_get_item_attribute (menu, i, G_MENU_ATTRIBUTE_ACTION, "s", &item_action);

      if (g_str_equal (action, item_action))
        func (menu, i);

      g_free (item_action);
    }
}

static void
im_phone_menu_update_toplevel (ImPhoneMenu *menu)
{
  if (g_menu_model_get_n_items (G_MENU_MODEL (menu->message_section)) ||
      g_menu_model_get_n_items (G_MENU_MODEL (menu->source_section)))
    {
      if (g_menu_model_get_n_items (G_MENU_MODEL (menu->toplevel_menu)) == 0)
        {
          GMenuItem *item;

          g_menu_append_section (menu->toplevel_menu, NULL, G_MENU_MODEL (menu->message_section));
          g_menu_append_section (menu->toplevel_menu, NULL, G_MENU_MODEL (menu->source_section));

          item = g_menu_item_new ("Clear All", "remove-all");
          g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.button");
          g_menu_append_item (menu->toplevel_menu, item);

          g_object_unref (item);
        }
    }
  else
    {
      while (g_menu_model_get_n_items (G_MENU_MODEL (menu->toplevel_menu)))
        g_menu_remove (menu->toplevel_menu, 0);
    }
}

static void
im_phone_menu_dispose (GObject *object)
{
  ImPhoneMenu *menu = IM_PHONE_MENU (object);

  g_clear_object (&menu->toplevel_menu);
  g_clear_object (&menu->message_section);
  g_clear_object (&menu->source_section);

  G_OBJECT_CLASS (im_phone_menu_parent_class)->dispose (object);
}

static void
im_phone_menu_finalize (GObject *object)
{
  G_OBJECT_CLASS (im_phone_menu_parent_class)->finalize (object);
}

static void
im_phone_menu_class_init (ImPhoneMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = im_phone_menu_dispose;
  object_class->finalize = im_phone_menu_finalize;
}

static void
im_phone_menu_init (ImPhoneMenu *menu)
{
  menu->toplevel_menu = g_menu_new ();
  menu->message_section = g_menu_new ();
  menu->source_section = g_menu_new ();

  g_signal_connect_swapped (menu->message_section, "items-changed",
                            G_CALLBACK (im_phone_menu_update_toplevel), menu);
  g_signal_connect_swapped (menu->source_section, "items-changed",
                            G_CALLBACK (im_phone_menu_update_toplevel), menu);

  im_phone_menu_update_toplevel (menu);
}

ImPhoneMenu *
im_phone_menu_new (void)
{
  return g_object_new (IM_TYPE_PHONE_MENU, NULL);
}

GMenuModel *
im_phone_menu_get_model (ImPhoneMenu *menu)
{
  g_return_val_if_fail (IM_IS_PHONE_MENU (menu), NULL);

  return G_MENU_MODEL (menu->toplevel_menu);
}

void
im_phone_menu_add_message (ImPhoneMenu     *menu,
                           const gchar     *app_id,
                           const gchar     *app_icon,
                           const gchar     *id,
                           const gchar     *iconstr,
                           const gchar     *title,
                           const gchar     *subtitle,
                           const gchar     *body,
                           GVariant        *actions,
                           gint64           time)
{
  GMenuItem *item;
  gchar *action_name;

  g_return_if_fail (IM_IS_PHONE_MENU (menu));
  g_return_if_fail (app_id);

  action_name = g_strconcat (app_id, ".msg.", id, NULL);

  item = g_menu_item_new (title, action_name);

  if (g_variant_n_children (actions))
    {
      g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.messages.snapdecision");
      g_menu_item_set_attribute (item, "x-canonical-message-actions", "v", actions);
    }
  else
    g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.messages.messageitem");

  g_menu_item_set_attribute (item, "x-canonical-message-id", "s", id);
  g_menu_item_set_attribute (item, "x-canonical-subtitle", "s", subtitle);
  g_menu_item_set_attribute (item, "x-canonical-text", "s", body);
  g_menu_item_set_attribute (item, "x-canonical-time", "x", time);

  if (iconstr)
    g_menu_item_set_attribute (item, "x-canonical-icon", "s", iconstr);

  if (app_icon)
    g_menu_item_set_attribute (item, "x-canonical-app-icon", "s", app_icon);

  g_menu_prepend_item (menu->message_section, item);

  g_free (action_name);
  g_object_unref (item);
}

void
im_phone_menu_remove_message (ImPhoneMenu     *menu,
                              const gchar     *app_id,
                              const gchar     *id)
{
  gchar *action_name;

  g_return_if_fail (IM_IS_PHONE_MENU (menu));
  g_return_if_fail (app_id != NULL);

  action_name = g_strconcat (app_id, ".msg.", id, NULL);
  im_phone_menu_foreach_item_with_action (G_MENU_MODEL (menu->message_section),
                                          action_name,
                                          (ImMenuForeachFunc) g_menu_remove);

  g_free (action_name);
}

void
im_phone_menu_add_source (ImPhoneMenu     *menu,
                          const gchar     *app_id,
                          const gchar     *id,
                          const gchar     *label,
                          const gchar     *iconstr)
{
  GMenuItem *item;
  gchar *action_name;

  g_return_if_fail (IM_IS_PHONE_MENU (menu));
  g_return_if_fail (app_id != NULL);

  action_name = g_strconcat (app_id, ".src.", id, NULL);

  item = g_menu_item_new (label, action_name);
  g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.messages.sourceitem");

  if (iconstr)
    g_menu_item_set_attribute (item, "x-canonical-icon", "s", iconstr);

  g_menu_prepend_item (menu->source_section, item);

  g_free (action_name);
  g_object_unref (item);
}

void
im_phone_menu_remove_source (ImPhoneMenu     *menu,
                             const gchar     *app_id,
                             const gchar     *id)
{
  gchar *action_name;

  g_return_if_fail (IM_IS_PHONE_MENU (menu));
  g_return_if_fail (app_id != NULL);

  action_name = g_strconcat (app_id, ".src.", id, NULL);
  im_phone_menu_foreach_item_with_action (G_MENU_MODEL (menu->source_section),
                                          action_name,
                                          (ImMenuForeachFunc) g_menu_remove);

  g_free (action_name);
}

static void
im_phone_menu_remove_all_for_app (GMenu           *menu,
                                  const gchar     *app_id)
{
  gchar *prefix;
  gint n_items;
  gint i = 0;

  prefix = g_strconcat (app_id, ".", NULL);

  n_items = g_menu_model_get_n_items (G_MENU_MODEL (menu));
  while (i < n_items)
    {
      gchar *action;

      g_menu_model_get_item_attribute (G_MENU_MODEL (menu), i, G_MENU_ATTRIBUTE_ACTION, "s", &action);
      if (g_str_has_prefix (action, prefix))
        {
          g_menu_remove (menu, i);
          n_items--;
        }
      else
        {
          i++;
        }

      g_free (action);
    }

  g_free (prefix);
}

void
im_phone_menu_remove_application (ImPhoneMenu     *menu,
                                  const gchar     *app_id)
{
  g_return_if_fail (IM_IS_PHONE_MENU (menu));
  g_return_if_fail (app_id != NULL);

  im_phone_menu_remove_all_for_app (menu->source_section, app_id);
  im_phone_menu_remove_all_for_app (menu->message_section, app_id);
}
