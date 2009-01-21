#ifndef __IM_MENU_ITEM_H__
#define __IM_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libindicate/listener.h>

G_BEGIN_DECLS

#define IM_MENU_ITEM_TYPE            (im_menu_item_get_type ())
#define IM_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_MENU_ITEM_TYPE, ImMenuItem))
#define IM_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IM_MENU_ITEM_TYPE, ImMenuItemClass))
#define IS_IM_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_MENU_ITEM_TYPE))
#define IS_IM_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IM_MENU_ITEM_TYPE))
#define IM_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IM_MENU_ITEM_TYPE, ImMenuItemClass))

typedef struct _ImMenuItem      ImMenuItem;
typedef struct _ImMenuItemClass ImMenuItemClass;

struct _ImMenuItemClass {
	GtkMenuItemClass parent_class;
};

struct _ImMenuItem {
	GtkMenuItem parent;
};

GType im_menu_item_get_type (void);
ImMenuItem * im_menu_item_new (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator);

G_END_DECLS

#endif

