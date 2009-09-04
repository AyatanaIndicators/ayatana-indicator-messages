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
#ifndef __IM_MENU_ITEM_H__
#define __IM_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libdbusmenu-glib/menuitem.h>
#include <libindicate/listener.h>

G_BEGIN_DECLS

#define IM_MENU_ITEM_TYPE            (im_menu_item_get_type ())
#define IM_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_MENU_ITEM_TYPE, ImMenuItem))
#define IM_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IM_MENU_ITEM_TYPE, ImMenuItemClass))
#define IS_IM_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_MENU_ITEM_TYPE))
#define IS_IM_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IM_MENU_ITEM_TYPE))
#define IM_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IM_MENU_ITEM_TYPE, ImMenuItemClass))

#define IM_MENU_ITEM_SIGNAL_TIME_CHANGED  "time-changed"

typedef struct _ImMenuItem      ImMenuItem;
typedef struct _ImMenuItemClass ImMenuItemClass;

struct _ImMenuItemClass {
	DbusmenuMenuitemClass parent_class;

	void (*time_changed) (glong seconds);
};

struct _ImMenuItem {
	DbusmenuMenuitem parent;
};

GType im_menu_item_get_type (void);
ImMenuItem * im_menu_item_new (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator);
glong im_menu_item_get_seconds (ImMenuItem * menuitem);

G_END_DECLS

#endif

