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

#ifndef __LAUNCHER_MENU_ITEM_H__
#define __LAUNCHER_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define LAUNCHER_MENU_ITEM_TYPE            (launcher_menu_item_get_type ())
#define LAUNCHER_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LAUNCHER_MENU_ITEM_TYPE, LauncherMenuItem))
#define LAUNCHER_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LAUNCHER_MENU_ITEM_TYPE, LauncherMenuItemClass))
#define IS_LAUNCHER_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LAUNCHER_MENU_ITEM_TYPE))
#define IS_LAUNCHER_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_MENU_ITEM_TYPE))
#define LAUNCHER_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LAUNCHER_MENU_ITEM_TYPE, LauncherMenuItemClass))

#define LAUNCHER_MENU_ITEM_SIGNAL_NAME_CHANGED   "name-changed"

typedef struct _LauncherMenuItem      LauncherMenuItem;
typedef struct _LauncherMenuItemClass LauncherMenuItemClass;

struct _LauncherMenuItemClass {
	DbusmenuMenuitemClass parent_class;

	void (* name_changed) (gchar * name);
};

struct _LauncherMenuItem {
	DbusmenuMenuitem parent;
};

GType launcher_menu_item_get_type (void);
LauncherMenuItem * launcher_menu_item_new (const gchar * desktop_file);
const gchar * launcher_menu_item_get_name (LauncherMenuItem * appitem);
void launcher_menu_item_set_eclipsed (LauncherMenuItem * li, gboolean eclipsed);

G_END_DECLS

#endif /* __LAUNCHER_MENU_ITEM_H__ */

