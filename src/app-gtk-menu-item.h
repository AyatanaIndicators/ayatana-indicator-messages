#ifndef __APP_GTK_MENU_ITEM_H__
#define __APP_GTK_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define APP_GTK_MENU_ITEM_TYPE            (app_gtk_menu_item_get_type ())
#define APP_GTK_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APP_GTK_MENU_ITEM_TYPE, AppGtkMenuItem))
#define APP_GTK_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APP_GTK_MENU_ITEM_TYPE, AppGtkMenuItemClass))
#define IS_APP_GTK_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APP_GTK_MENU_ITEM_TYPE))
#define IS_APP_GTK_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APP_GTK_MENU_ITEM_TYPE))
#define APP_GTK_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), APP_GTK_MENU_ITEM_TYPE, AppGtkMenuItemClass))

typedef struct _AppGtkMenuItem      AppGtkMenuItem;
typedef struct _AppGtkMenuItemClass AppGtkMenuItemClass;

struct _AppGtkMenuItemClass {
	GtkCheckMenuItemClass parent_class;
};

struct _AppGtkMenuItem {
	GtkCheckMenuItem parent;
};

GType app_gtk_menu_item_get_type (void);

G_END_DECLS

#endif
