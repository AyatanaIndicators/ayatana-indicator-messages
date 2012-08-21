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

#ifndef __IM_APP_MENU_ITEM_H__
#define __IM_APP_MENU_ITEM_H__

#include <gtk/gtk.h>

#define IM_TYPE_APP_MENU_ITEM            (im_app_menu_item_get_type ())
#define IM_APP_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_TYPE_APP_MENU_ITEM, ImAppMenuItem))
#define IM_APP_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IM_TYPE_APP_MENU_ITEM, ImAppMenuItemClass))
#define IS_IM_APP_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_TYPE_APP_MENU_ITEM))
#define IS_IM_APP_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IM_TYPE_APP_MENU_ITEM))
#define IM_APP_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IM_TYPE_APP_MENU_ITEM, ImAppMenuItemClass))

typedef struct _ImAppMenuItem        ImAppMenuItem;
typedef struct _ImAppMenuItemClass   ImAppMenuItemClass;
typedef struct _ImAppMenuItemPrivate ImAppMenuItemPrivate;

struct _ImAppMenuItemClass
{
  GtkMenuItemClass parent_class;
};

struct _ImAppMenuItem
{
  GtkMenuItem parent;
  ImAppMenuItemPrivate *priv;
};

GType           im_app_menu_item_get_type           (void);

void            im_app_menu_item_set_menu_item      (ImAppMenuItem *item,
                                                     GMenuItem     *menuitem);
void            im_app_menu_item_set_action_group   (ImAppMenuItem *self,
                                                     GActionGroup  *action_group);

#endif
