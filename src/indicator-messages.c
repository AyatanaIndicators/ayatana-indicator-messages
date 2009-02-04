
#include <string.h>
#include <gtk/gtk.h>
#include <libindicate/listener.h>

#include "im-menu-item.h"

static IndicateListener * listener;
static GList * imList;
#if 0
static GHashTable * mailHash;
#endif

typedef struct _imList_t imList_t;
struct _imList_t {
	IndicateListenerServer * server;
	IndicateListenerIndicator * indicator;
	GtkWidget * menuitem;
};

static gboolean
imList_equal (gconstpointer a, gconstpointer b)
{
	imList_t * pa, * pb;

	pa = (imList_t *)a;
	pb = (imList_t *)b;

	gchar * pas = (gchar *)pa->server;
	gchar * pbs = (gchar *)pb->server;

	guint pai = GPOINTER_TO_UINT(pa->indicator);
	guint pbi = GPOINTER_TO_UINT(pb->indicator);

	g_debug("\tComparing (%s %d) to (%s %d)", pas, pai, pbs, pbi);

	return !((!strcmp(pas, pbs)) && (pai == pbi));
}

static void
subtype_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	GtkMenuShell * menushell = GTK_MENU_SHELL(data);
	if (menushell == NULL) {
		g_error("Data in callback is not a menushell");
		return;
	}

	if (property == NULL || strcmp(property, "subtype")) {
		/* We should only ever get subtypes, but just in case */
		g_warning("Subtype callback got a property '%s'", property);
		return;
	}

	if (propertydata == NULL || propertydata[0] == '\0') {
		/* It's possible that this message didn't have a subtype.  That's
		 * okay, but we don't want to display those */
		g_debug("No subtype");
		return;
	}

	g_debug("Message subtype: %s", propertydata);

	if (!strcmp(propertydata, "im")) {
		imList_t * listItem = g_new(imList_t, 1);
		listItem->server = server;
		listItem->indicator = indicator;

		g_debug("Building IM Item");
		ImMenuItem * menuitem = im_menu_item_new(listener, server, indicator);
		g_object_ref(G_OBJECT(menuitem));
		listItem->menuitem = GTK_WIDGET(menuitem);

		g_debug("Adding to IM Hash");
		imList = g_list_append(imList, listItem);

		g_debug("Placing in Shell");
		gtk_menu_shell_prepend(menushell, GTK_WIDGET(menuitem));
#if 0
	} else if (!strcmp(propertydata, "mail")) {
		gpointer pntr_menu_item;
		pntr_menu_item = g_hash_table_lookup(mailHash, server);
		if (pntr_menu_item == NULL) {
			/* If we don't know about it, we need a new menu item */
			GtkWidget * menuitem = mail_menu_item_new(listener, server);
			g_object_ref(menuitem);

			g_hash_table_insert(mailHash, server, menuitem);

			gtk_menu_shell_append(menushell, menuitem);
		} else {
			/* If we do, we need to increment the count */
			MailMenuItem * menu_item = MAIL_MENU_ITEM(pntr_menu_item);
			mail_menu_item_increment(menu_item);
		}
#endif
	}

	return;
}

static void
indicator_added (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * type, gpointer data)
{
	if (type == NULL || strcmp(type, "message")) {
		/* We only care about message type indicators
		   all of the others can go to the bit bucket */
		g_debug("Ignoreing indicator of type '%s'", type);
		return;
	}
	g_debug("Got a message");

	indicate_listener_get_property(listener, server, indicator, "subtype", subtype_cb, data);	
	return;
}

static void
indicator_removed (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * type, gpointer data)
{
	g_debug("Removing %s %d", (gchar*)server, (guint)indicator);
	if (type == NULL || strcmp(type, "message")) {
		/* We only care about message type indicators
		   all of the others can go to the bit bucket */
		g_debug("Ignoreing indicator of type '%s'", type);
		return;
	}

	gboolean removed = FALSE;

	/* Look in the IM Hash Table */
	imList_t listData;
	listData.server = server;
	listData.indicator = indicator;

	GList * listItem = g_list_find_custom(imList, &listData, imList_equal);
	GtkWidget * menuitem = NULL;
	if (listItem != NULL) {
		menuitem = ((imList_t *)listItem->data)->menuitem;
	}

	if (!removed && menuitem != NULL) {
		g_object_ref(menuitem);
		imList = g_list_remove(imList, listItem->data);

		gtk_widget_hide(menuitem);
		gtk_container_remove(GTK_CONTAINER(data), menuitem);

		g_object_unref(menuitem);
		removed = TRUE;
	}

	/* TODO: Look at mail */

	if (!removed) {
		g_warning("We were asked to remove %s %d but we didn't.", (gchar*)server, (guint)indicator);
	}

	return;
}

GtkWidget *
get_menu_item (void)
{
	listener = indicate_listener_new();
	imList = NULL;
#if 0
	mailHash = g_hash_table_new_full(g_direct_hash, g_direct_equal,
	                               NULL, g_object_unref);
#endif

	GtkWidget * mainmenu = gtk_menu_item_new();

	GtkWidget * image = gtk_image_new_from_icon_name("indicator-messages", GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(mainmenu), image);

	GtkWidget * submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mainmenu), submenu);
	gtk_widget_show(submenu);
	gtk_widget_show(mainmenu);

	g_signal_connect(listener, "indicator-added", G_CALLBACK(indicator_added), submenu);
	g_signal_connect(listener, "indicator-removed", G_CALLBACK(indicator_removed), submenu);

	return mainmenu;
}

