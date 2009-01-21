#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "im-menu-item.h"

typedef struct _ImMenuItemPrivate ImMenuItemPrivate;

struct _ImMenuItemPrivate
{
	IndicateListener *           listener;
	IndicateListenerSever *      server;
	IndicateListenerIndicator *  indicator;

	GtkHBox * hbox;
	GtkLabel * user;
	GtkLabel * time;
	GtkImage * icon;
};

#define IM_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_MENU_ITEM_TYPE, ImMenuItemPrivate))

/* Prototypes */
static void im_menu_item_class_init (ImMenuItemClass *klass);
static void im_menu_item_init       (ImMenuItem *self);
static void im_menu_item_dispose    (GObject *object);
static void im_menu_item_finalize   (GObject *object);
static void sender_cb               (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     gchar * propertydata,
                                     gpointer data);
static void time_cb                 (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     gchar * propertydata,
                                     gpointer data);
static void icon_cb                 (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     gchar * propertydata,
                                     gpointer data);


static GtkSizeGroup * icon_group = NULL;
static GtkSizeGroup * user_group = NULL;
static GtkSizeGroup * time_group = NULL;


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
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = NULL;
	priv->server = NULL;
	priv->indicator = NULL;

	/* build widgets first */
	priv->icon = gtk_image_new();
	priv->user = gtk_label_new("");
	priv->time = gtk_label_new("");

	if (icon_group == NULL) {
		icon_group = gtk_size_group_new(GTK_SIZE_GROUP_MODE_HORIZONTAL);
	}
	if (user_group == NULL) {
		user_group = gtk_size_group_new(GTK_SIZE_GROUP_MODE_HORIZONTAL);
	}
	if (time_group == NULL) {
		time_group = gtk_size_group_new(GTK_SIZE_GROUP_MODE_HORIZONTAL);
	}
	gtk_size_group_add_widget(icon_group, priv->icon);
	gtk_size_group_add_widget(user_group, priv->user);
	gtk_size_group_add_widget(time_group, priv->time);

	priv->hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack(GTK_BOX(priv->hbox), GTK_WIDGET(priv->time), FALSE, TRUE, 3);
	gtk_box_pack(GTK_BOX(priv->hbox), GTK_WIDGET(priv->user), TRUE,  TRUE, 3);
	gtk_box_pack(GTK_BOX(priv->hbox), GTK_WIDGET(priv->icon), FALSE, TRUE, 3);

	gtk_container_add(GTK_CONTAINER(self), priv->hbox);

	return;
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

static void
icon_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{


}

static void
time_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	ImMenuItem * self = IM_MENU_ITEM(data);
	if (self == NULL) {
		g_error("Menu Item callback called without a menu item");
		return;
	}

	if (property == NULL || strcmp(property, "sender")) {
		g_warning("Sender callback called without being send the sender.");
		return;
	}

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	gtk_label_set_label(priv->time, propertydata);
	gtk_widget_show(priv->time);

	return;
}

static void
sender_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	ImMenuItem * self = IM_MENU_ITEM(data);
	if (self == NULL) {
		g_error("Menu Item callback called without a menu item");
		return;
	}

	if (property == NULL || strcmp(property, "sender")) {
		g_warning("Sender callback called without being send the sender.");
		return;
	}

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	gtk_label_set_label(priv->user, propertydata);
	gtk_widget_show(priv->user);

	/* Once we have the user we'll show the menu item */
	gtk_widget_show(self);

	return;
}

ImMenuItem *
im_menu_item_new (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator)
{
	ImMenuItem * self = g_object_new(IM_MENU_ITEM_TYPE, NULL);

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = listener;
	priv->server = server;
	priv->indicator = indicator;


	indicate_listener_get_property(listener, server, indicator, "sender", sender_cb, self);	
	indicate_listener_get_property(listener, server, indicator, "time",   time_cb, self);	
	indicate_listener_get_property(listener, server, indicator, "icon",   icon_cb, self);	

	return;
}
