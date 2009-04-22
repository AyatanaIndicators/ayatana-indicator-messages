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

#include <string.h>
#include <gtk/gtk.h>
#include <libindicate/listener.h>

#include <libindicator/indicator.h>
INDICATOR_SET_VERSION

#include "im-menu-item.h"
#include "app-menu-item.h"

static IndicateListener * listener;
static GList * serverList;
static GtkWidget * main_image;
static GtkWidget * main_menu;

static void server_count_changed (AppMenuItem * appitem, guint count, gpointer data);
static void server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data);
static void im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data);
static void reconsile_list_and_menu (GList * serverlist, GtkMenuShell * menushell);
static void indicator_removed (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * type, gpointer data);

#define DESIGN_TEAM_SIZE  design_team_size
static GtkIconSize design_team_size;

typedef struct _serverList_t serverList_t;
struct _serverList_t {
	IndicateListenerServer * server;
	AppMenuItem * menuitem;
	GList * imList;
};

static gint
serverList_equal (gconstpointer a, gconstpointer b)
{
	serverList_t * pa, * pb;

	pa = (serverList_t *)a;
	pb = (serverList_t *)b;

	const gchar * pas = INDICATE_LISTENER_SERVER_DBUS_NAME(pa->server);
	const gchar * pbs = INDICATE_LISTENER_SERVER_DBUS_NAME(pb->server);

	return g_strcmp0(pas, pbs);
}

static gint
serverList_sort (gconstpointer a, gconstpointer b)
{
	serverList_t * pa, * pb;

	pa = (serverList_t *)a;
	pb = (serverList_t *)b;

	const gchar * pan = app_menu_item_get_name(pa->menuitem);
	const gchar * pbn = app_menu_item_get_name(pb->menuitem);

	return g_strcmp0(pan, pbn);
}

typedef struct _imList_t imList_t;
struct _imList_t {
	IndicateListenerServer * server;
	IndicateListenerIndicator * indicator;
	GtkWidget * menuitem;
	gulong timechange_cb;
};

static gboolean
imList_equal (gconstpointer a, gconstpointer b)
{
	imList_t * pa, * pb;

	pa = (imList_t *)a;
	pb = (imList_t *)b;

	const gchar * pas = INDICATE_LISTENER_SERVER_DBUS_NAME(pa->server);
	const gchar * pbs = INDICATE_LISTENER_SERVER_DBUS_NAME(pb->server);

	guint pai = INDICATE_LISTENER_INDICATOR_ID(pa->indicator);
	guint pbi = INDICATE_LISTENER_INDICATOR_ID(pb->indicator);

	g_debug("\tComparing (%s %d) to (%s %d)", pas, pai, pbs, pbi);

	return !((!g_strcmp0(pas, pbs)) && (pai == pbi));
}

static gint
imList_sort (gconstpointer a, gconstpointer b)
{
	imList_t * pa, * pb;

	pa = (imList_t *)a;
	pb = (imList_t *)b;

	return (gint)(im_menu_item_get_seconds(IM_MENU_ITEM(pb->menuitem)) - im_menu_item_get_seconds(IM_MENU_ITEM(pa->menuitem)));
}

static void 
server_added (IndicateListener * listener, IndicateListenerServer * server, gchar * type, gpointer data)
{
	g_debug("Server Added '%s' of type '%s'.", INDICATE_LISTENER_SERVER_DBUS_NAME(server), type);
	if (type == NULL) {
		return;
	}

	if (type[0] == '\0') {
		return;
	}

	if (strncmp(type, "message", strlen("message"))) {
		g_debug("\tServer type '%s' is not a message based type.", type);
		return;
	}

	GtkMenuShell * menushell = GTK_MENU_SHELL(data);
	if (menushell == NULL) {
		g_error("\tData in callback is not a menushell");
		return;
	}

	AppMenuItem * menuitem = app_menu_item_new(listener, server);
	g_signal_connect(G_OBJECT(menuitem), APP_MENU_ITEM_SIGNAL_COUNT_CHANGED, G_CALLBACK(server_count_changed), NULL);
	g_signal_connect(G_OBJECT(menuitem), APP_MENU_ITEM_SIGNAL_NAME_CHANGED,  G_CALLBACK(server_name_changed),  menushell);

	serverList_t * sl_item = g_new0(serverList_t, 1);
	sl_item->server = server;
	sl_item->menuitem = menuitem;
	sl_item->imList = NULL;

	/* Incase we got an indicator first */
	GList * alreadythere = g_list_find_custom(serverList, sl_item, serverList_equal);
	if (alreadythere != NULL) {
		g_free(sl_item);
		sl_item = (serverList_t *)alreadythere->data;
		sl_item->menuitem = menuitem;
		serverList = g_list_sort(serverList, serverList_sort);
	} else {
		serverList = g_list_insert_sorted(serverList, sl_item, serverList_sort);
	}

	gtk_menu_shell_prepend(menushell, GTK_WIDGET(menuitem));
	gtk_widget_show(GTK_WIDGET(menuitem));
	gtk_widget_show(GTK_WIDGET(main_menu));

	reconsile_list_and_menu(serverList, menushell);

	return;
}

static void
server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data)
{
	serverList = g_list_sort(serverList, serverList_sort);
	reconsile_list_and_menu(serverList, GTK_MENU_SHELL(data));
	return;
}

static void
server_count_changed (AppMenuItem * appitem, guint count, gpointer data)
{
	static gboolean showing_new_icon = FALSE;

	/* Quick check for a common case */
	if (count != 0 && showing_new_icon) {
		return;
	}

	/* Odd that we'd get a signal in this case, but let's
	   take it out of the mix too */
	if (count == 0 && !showing_new_icon) {
		return;
	}

	if (count != 0) {
		g_debug("Setting image to 'new'");
		showing_new_icon = TRUE;
		gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages-new", DESIGN_TEAM_SIZE);
		return;
	}

	/* Okay, now at this point the count is zero and it
	   might result in a switching of the icon back to being
	   the plain one.  Let's check. */

	gboolean we_have_indicators = FALSE;
	GList * appitems = serverList;
	for (; appitems != NULL; appitems = appitems->next) {
		AppMenuItem * appitem = ((serverList_t *)appitems->data)->menuitem;
		if (app_menu_item_get_count(appitem) != 0) {
			we_have_indicators = TRUE;
			break;
		}
	}

	if (!we_have_indicators) {
		g_debug("Setting image to boring");
		showing_new_icon = FALSE;
		gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages", DESIGN_TEAM_SIZE);
	}

	return;
}

static void
im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data)
{
	serverList_t * sl = (serverList_t *)data;
	sl->imList = g_list_sort(sl->imList, imList_sort);
	reconsile_list_and_menu(serverList, GTK_MENU_SHELL(gtk_widget_get_parent(GTK_WIDGET(imitem))));
	return;
}

static void 
server_removed (IndicateListener * listener, IndicateListenerServer * server, gchar * type, gpointer data)
{
	g_debug("Removing server: %s", INDICATE_LISTENER_SERVER_DBUS_NAME(server));
	serverList_t slt;
	slt.server = server;
	GList * lookup = g_list_find_custom(serverList, &slt, serverList_equal);

	if (lookup == NULL) {
		g_debug("\tUnable to find server: %s", INDICATE_LISTENER_SERVER_DBUS_NAME(server));
		return;
	}

	serverList_t * sltp = (serverList_t *)lookup->data;

	while (sltp->imList) {
		imList_t * imitem = (imList_t *)sltp->imList->data;
		indicator_removed(listener, server, imitem->indicator, "message", data);
	}

	serverList = g_list_remove(serverList, sltp);

	if (sltp->menuitem != NULL) {
		gtk_widget_hide(GTK_WIDGET(sltp->menuitem));
		gtk_container_remove(GTK_CONTAINER(data), GTK_WIDGET(sltp->menuitem));
	}

	g_free(sltp);

	if (g_list_length(serverList) == 0) {
		gtk_widget_hide(main_menu);
	} else {
		/* Simulate a server saying zero to recalculate icon */
		server_count_changed(NULL, 0, NULL);
	}

	return;
}

typedef struct _menushell_location menushell_location_t;
struct _menushell_location {
	const IndicateListenerServer * server;
	gint position;
	gboolean found;
};

static void
menushell_foreach_cb (GtkWidget * data_mi, gpointer data_ms) {
	menushell_location_t * msl = (menushell_location_t *)data_ms;

	if (msl->found) return;

	msl->position++;

	if (!IS_APP_MENU_ITEM(data_mi)) {
		return;
	}

	AppMenuItem * appmenu = APP_MENU_ITEM(data_mi);
	if (!g_strcmp0(INDICATE_LISTENER_SERVER_DBUS_NAME((IndicateListenerServer*)msl->server), INDICATE_LISTENER_SERVER_DBUS_NAME(app_menu_item_get_server(appmenu)))) {
		msl->found = TRUE;
	}

	return;
}

static void
reconsile_list_and_menu (GList * serverlist, GtkMenuShell * menushell)
{
	guint position = 0;
	GList * serverentry;

	g_debug("Reordering Menu:");

	for (serverentry = serverList; serverentry != NULL; serverentry = serverentry->next) {
		serverList_t * si = (serverList_t *)serverentry->data;
		if (si->menuitem != NULL) {
			g_debug("\tMoving app %s to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(si->server), position);
			gtk_menu_reorder_child(GTK_MENU(menushell), GTK_WIDGET(si->menuitem), position);
			position++;
		}

		GList * imentry;
		for (imentry = si->imList; imentry != NULL; imentry = imentry->next) {
			imList_t * imi = (imList_t *)imentry->data;

			if (imi->menuitem != NULL) {
				g_debug("\tMoving indicator on %s id %d to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(imi->server), INDICATE_LISTENER_INDICATOR_ID(imi->indicator), position);
				gtk_menu_reorder_child(GTK_MENU(menushell), imi->menuitem, position);
				position++;
			}
		}
	}

	return;
}

static void
subtype_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	GtkMenuShell * menushell = GTK_MENU_SHELL(data);
	if (menushell == NULL) {
		g_error("Data in callback is not a menushell");
		return;
	}

	if (property == NULL || g_strcmp0(property, "subtype")) {
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

	if (!g_strcmp0(propertydata, "im") || !g_strcmp0(propertydata, "login")) {
		imList_t * listItem = g_new0(imList_t, 1);
		listItem->server = server;
		listItem->indicator = indicator;

		g_debug("Building IM Item");
		ImMenuItem * menuitem = im_menu_item_new(listener, server, indicator, !g_strcmp0(propertydata, "im"));
		g_object_ref(G_OBJECT(menuitem));
		listItem->menuitem = GTK_WIDGET(menuitem);

		g_debug("Finding the server entry");
		serverList_t sl_item_local;
		serverList_t * sl_item = NULL;
		sl_item_local.server = server;
		GList * serverentry = g_list_find_custom(serverList, &sl_item_local, serverList_equal);

		if (serverentry == NULL) {
			/* This sucks, we got an indicator before the server.  I guess
			   that's the joy of being asynchronous */
			serverList_t * sl_item = g_new0(serverList_t, 1);
			sl_item->server = server;
			sl_item->menuitem = NULL;
			sl_item->imList = NULL;

			serverList = g_list_insert_sorted(serverList, sl_item, serverList_sort);
		} else {
			sl_item = (serverList_t *)serverentry->data;
		}

		g_debug("Adding to IM List");
		sl_item->imList = g_list_insert_sorted(sl_item->imList, listItem, imList_sort);
		listItem->timechange_cb = g_signal_connect(G_OBJECT(menuitem), IM_MENU_ITEM_SIGNAL_TIME_CHANGED, G_CALLBACK(im_time_changed), sl_item);

		g_debug("Placing in Shell");
		menushell_location_t msl;
		msl.found = FALSE;
		msl.position = 0;
		msl.server = server;

		gtk_container_foreach(GTK_CONTAINER(menushell), menushell_foreach_cb, &msl);
		if (msl.found) {
			gtk_menu_shell_insert(menushell, GTK_WIDGET(menuitem), msl.position);
		} else {
			g_warning("Unable to find server menu item");
			gtk_menu_shell_append(menushell, GTK_WIDGET(menuitem));
		}
	}

	return;
}

static void
indicator_added (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * type, gpointer data)
{
	if (type == NULL || g_strcmp0(type, "message")) {
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
	g_debug("Removing %s %d", INDICATE_LISTENER_SERVER_DBUS_NAME(server), INDICATE_LISTENER_INDICATOR_ID(indicator));
	if (type == NULL || g_strcmp0(type, "message")) {
		/* We only care about message type indicators
		   all of the others can go to the bit bucket */
		g_debug("Ignoreing indicator of type '%s'", type);
		return;
	}

	gboolean removed = FALSE;

	serverList_t sl_item_local;
	serverList_t * sl_item = NULL;
	sl_item_local.server = server;
	GList * serverentry = g_list_find_custom(serverList, &sl_item_local, serverList_equal);
	if (serverentry == NULL) {
		return;
	}
	sl_item = (serverList_t *)serverentry->data;

	/* Look in the IM Hash Table */
	imList_t listData;
	listData.server = server;
	listData.indicator = indicator;

	GList * listItem = g_list_find_custom(sl_item->imList, &listData, imList_equal);
	GtkWidget * menuitem = NULL;
	imList_t * ilt = NULL;
	if (listItem != NULL) {
		ilt = (imList_t *)listItem->data;
		menuitem = ilt->menuitem;
	}

	if (!removed && menuitem != NULL) {
		sl_item->imList = g_list_remove(sl_item->imList, ilt);
		g_signal_handler_disconnect(menuitem, ilt->timechange_cb);
		g_free(ilt);

		gtk_widget_hide(menuitem);
		gtk_container_remove(GTK_CONTAINER(data), menuitem);
		removed = TRUE;
	}

	if (!removed) {
		g_warning("We were asked to remove %s %d but we didn't.", INDICATE_LISTENER_SERVER_DBUS_NAME(server), INDICATE_LISTENER_INDICATOR_ID(indicator));
	}

	return;
}

GtkLabel *
get_label (void)
{
	return NULL;
}

GtkImage *
get_icon (void)
{
	design_team_size = gtk_icon_size_register("design-team-size", 22, 22);

	main_image = gtk_image_new_from_icon_name("indicator-messages", DESIGN_TEAM_SIZE);
	gtk_widget_show(main_image);

	return GTK_IMAGE(main_image);
}

GtkMenu *
get_menu (void)
{
	listener = indicate_listener_new();
	serverList = NULL;

	GtkWidget * submenu = gtk_menu_new();
	gtk_widget_show(submenu);

	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_INDICATOR_ADDED, G_CALLBACK(indicator_added), submenu);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_INDICATOR_REMOVED, G_CALLBACK(indicator_removed), submenu);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_SERVER_ADDED, G_CALLBACK(server_added), submenu);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_SERVER_REMOVED, G_CALLBACK(server_removed), submenu);

	return GTK_MENU(submenu);
}

