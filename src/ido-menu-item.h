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

#ifndef __IDO_MENU_ITEM_H__
#define __IDO_MENU_ITEM_H__

#include <gtk/gtk.h>

#define IDO_TYPE_MENU_ITEM            (ido_menu_item_get_type ())
#define IDO_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IDO_TYPE_MENU_ITEM, IdoMenuItem))
#define IDO_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IDO_TYPE_MENU_ITEM, IdoMenuItemClass))
#define IS_IDO_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IDO_TYPE_MENU_ITEM))
#define IS_IDO_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IDO_TYPE_MENU_ITEM))
#define IDO_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IDO_TYPE_MENU_ITEM, IdoMenuItemClass))

typedef struct _IdoMenuItem        IdoMenuItem;
typedef struct _IdoMenuItemClass   IdoMenuItemClass;
typedef struct _IdoMenuItemPrivate IdoMenuItemPrivate;

struct _IdoMenuItemClass
{
  GtkCheckMenuItemClass parent_class;
};

struct _IdoMenuItem
{
  GtkCheckMenuItem parent;
  IdoMenuItemPrivate *priv;
};

GType           ido_menu_item_get_type           (void);

void            ido_menu_item_set_menu_item      (IdoMenuItem *item,
                                                  GMenuItem   *menuitem);
void            ido_menu_item_set_action_group   (IdoMenuItem  *self,
                                                  GActionGroup *action_group);

#endif
