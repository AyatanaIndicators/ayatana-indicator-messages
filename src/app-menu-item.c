#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "app-menu-item.h"

typedef struct _AppMenuItemPrivate AppMenuItemPrivate;

struct _AppMenuItemPrivate
{
	IndicateListener *            listener;
	IndicateListenerServer *      server;

	GtkWidget * name;
};

#define APP_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), APP_MENU_ITEM_TYPE, AppMenuItemPrivate))

/* Prototypes */
static void app_menu_item_class_init (AppMenuItemClass *klass);
static void app_menu_item_init       (AppMenuItem *self);
static void app_menu_item_dispose    (GObject *object);
static void app_menu_item_finalize   (GObject *object);
static void activate_cb (AppMenuItem * self, gpointer data);



G_DEFINE_TYPE (AppMenuItem, app_menu_item, GTK_TYPE_MENU_ITEM);

static void
app_menu_item_class_init (AppMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AppMenuItemPrivate));

	object_class->dispose = app_menu_item_dispose;
	object_class->finalize = app_menu_item_finalize;
}

static void
app_menu_item_init (AppMenuItem *self)
{
	g_debug("Building new IM Menu Item");
	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = NULL;
	priv->server = NULL;
	priv->name = NULL;


	return;
}

static void
app_menu_item_dispose (GObject *object)
{
	G_OBJECT_CLASS (app_menu_item_parent_class)->dispose (object);
}

static void
app_menu_item_finalize (GObject *object)
{
	G_OBJECT_CLASS (app_menu_item_parent_class)->finalize (object);
}

AppMenuItem *
app_menu_item_new (IndicateListener * listener, IndicateListenerServer * server)
{
	g_debug("Building a new IM Menu Item");
	AppMenuItem * self = g_object_new(APP_MENU_ITEM_TYPE, NULL);

	AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = listener;
	priv->server = server;

	priv->name = gtk_label_new(INDICATE_LISTENER_SERVER_DBUS_NAME(server));
	gtk_widget_show(GTK_WIDGET(priv->name));

	gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(priv->name));

	g_signal_connect(G_OBJECT(self), "activate", G_CALLBACK(activate_cb), NULL);

	return self;
}

static void
activate_cb (AppMenuItem * self, gpointer data)
{
	//AppMenuItemPrivate * priv = APP_MENU_ITEM_GET_PRIVATE(self);

}
