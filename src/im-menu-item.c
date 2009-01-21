#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "im-menu-item.h"

typedef struct _ImMenuItemPrivate ImMenuItemPrivate;

struct _ImMenuItemPrivate
{
};

#define IM_MENU_ITEM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_MENU_ITEM_TYPE, ImMenuItemPrivate))

static void im_menu_item_class_init (ImMenuItemClass *klass);
static void im_menu_item_init       (ImMenuItem *self);
static void im_menu_item_dispose    (GObject *object);
static void im_menu_item_finalize   (GObject *object);

G_DEFINE_TYPE (ImMenuItem, im_menu_item, GTK_TYPE_MENU_ITEM);

static void
im_menu_item_class_init (ImMenuItemClass *klass)
{
GObjectClass *object_class = G_OBJECT_CLASS (klass);

g_type_class_add_private (klass, sizeof (ImMenuItemPrivate));

object_class->dispose = im_menu_item_dispose;
object_class->finalize = im_menu_item_finalize;
}

static void
im_menu_item_init (ImMenuItem *self)
{
}

static void
im_menu_item_dispose (GObject *object)
{
G_OBJECT_CLASS (im_menu_item_parent_class)->dispose (object);
}

static void
im_menu_item_finalize (GObject *object)
{
G_OBJECT_CLASS (im_menu_item_parent_class)->finalize (object);
}
