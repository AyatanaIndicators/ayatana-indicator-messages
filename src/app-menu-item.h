/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2009 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __APP_MENU_ITEM_H__
#define __APP_MENU_ITEM_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define APP_MENU_ITEM_TYPE            (app_menu_item_get_type ())
#define APP_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APP_MENU_ITEM_TYPE, AppMenuItem))
#define APP_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APP_MENU_ITEM_TYPE, AppMenuItemClass))
#define IS_APP_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APP_MENU_ITEM_TYPE))
#define IS_APP_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APP_MENU_ITEM_TYPE))
#define APP_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), APP_MENU_ITEM_TYPE, AppMenuItemClass))

typedef struct _AppMenuItem      AppMenuItem;
typedef struct _AppMenuItemClass AppMenuItemClass;

struct _AppMenuItemClass {
	GObjectClass parent_class;
};

struct _AppMenuItem {
	GObject parent;
};

GType app_menu_item_get_type (void);
AppMenuItem * app_menu_item_new (GDesktopAppInfo *appinfo);
guint app_menu_item_get_count (AppMenuItem * appitem);
const gchar * app_menu_item_get_name (AppMenuItem * appitem);
const gchar * app_menu_item_get_desktop (AppMenuItem * appitem);
GMenuModel * app_menu_item_get_menu (AppMenuItem *appitem);
GAppInfo * app_menu_item_get_app_info (AppMenuItem *appitem);

G_END_DECLS

#endif /* __APP_MENU_ITEM_H__ */

