#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app-gtk-menu-item.h"

typedef struct _AppGtkMenuItemPrivate AppGtkMenuItemPrivate;
struct _AppGtkMenuItemPrivate {
	int dummy;
};

#define APP_GTK_MENU_ITEM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), APP_GTK_MENU_ITEM_TYPE, AppGtkMenuItemPrivate))

static void app_gtk_menu_item_class_init (AppGtkMenuItemClass *klass);
static void app_gtk_menu_item_init       (AppGtkMenuItem *self);
static void app_gtk_menu_item_dispose    (GObject *object);
static void app_gtk_menu_item_finalize   (GObject *object);

G_DEFINE_TYPE (AppGtkMenuItem, app_gtk_menu_item, GTK_TYPE_CHECK_MENU_ITEM);

static void
app_gtk_menu_item_class_init (AppGtkMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AppGtkMenuItemPrivate));

	object_class->dispose = app_gtk_menu_item_dispose;
	object_class->finalize = app_gtk_menu_item_finalize;

	return;
}

static void
app_gtk_menu_item_init (AppGtkMenuItem *self)
{

	return;
}

static void
app_gtk_menu_item_dispose (GObject *object)
{

	G_OBJECT_CLASS (app_gtk_menu_item_parent_class)->dispose (object);
	return;
}

static void
app_gtk_menu_item_finalize (GObject *object)
{


	G_OBJECT_CLASS (app_gtk_menu_item_parent_class)->finalize (object);
	return;
}
