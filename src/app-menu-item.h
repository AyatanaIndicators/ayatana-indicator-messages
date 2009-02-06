#ifndef __APP_MENU_ITEM_H__
#define __APP_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libindicate/listener.h>

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
	GtkMenuItemClass parent_class;
};

struct _AppMenuItem {
	GtkMenuItem parent;
};

GType app_menu_item_get_type (void);
AppMenuItem * app_menu_item_new (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator);

G_END_DECLS

#endif

