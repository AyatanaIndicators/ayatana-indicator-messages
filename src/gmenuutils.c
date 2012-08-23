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

#include "gmenuutils.h"
#include "dbus-data.h"

/* g_menu_find_section:
 * @menu: a #GMenu
 * @section: the section to be found in @menu
 *
 * @Returns the index of the first menu item that is linked to #section, or -1
 * if there's no such item.
 */
int
g_menu_find_section (GMenu      *menu,
                     GMenuModel *section)
{
  GMenuModel *model = G_MENU_MODEL (menu);
  int n_items;
  int i;

  g_return_val_if_fail (G_IS_MENU_MODEL (section), -1);

  n_items = g_menu_model_get_n_items (model);
  for (i = 0; i < n_items; i++)
    {
      if (section == g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION))
        return i;
    }

  return -1;
}


void
g_menu_append_with_icon (GMenu *menu,
                         const gchar *label,
                         GIcon *icon,
                         const gchar *detailed_action)
{
  gchar *iconstr;

  iconstr = g_icon_to_string (icon);
  g_menu_append_with_icon_name (menu, label, iconstr, detailed_action);

  g_free (iconstr);
}

void
g_menu_append_with_icon_name (GMenu *menu,
                              const gchar *label,
                              const gchar *icon_name,
                              const gchar *detailed_action)
{
  GMenuItem *item;

  item = g_menu_item_new (label, detailed_action);
  g_menu_item_set_attribute (item, "x-canonical-icon", "s", icon_name);

  g_menu_append_item (menu, item);

  g_object_unref (item);
}
