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
#include <pango/pango-utils.h>
#include <dbus/dbus-glib-bindings.h>
#include <libindicate/listener.h>
#include <gio/gio.h>

#include <libdbusmenu-glib/server.h>

#include "im-menu-item.h"
#include "app-menu-item.h"
#include "launcher-menu-item.h"
#include "dbus-data.h"
#include "dirs.h"

static IndicateListener * listener;
static GList * serverList = NULL;
static GList * launcherList = NULL;

static DbusmenuMenuitem * root_menuitem = NULL;
static GMainLoop * mainloop = NULL;


static void server_count_changed (AppMenuItem * appitem, guint count, gpointer data);
static void server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data);
static void im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data);
static void resort_menu (DbusmenuMenuitem * menushell);
static void indicator_removed (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * type, gpointer data);
static void check_eclipses (AppMenuItem * ai);
static void remove_eclipses (AppMenuItem * ai);
static gboolean build_launcher (gpointer data);
static gboolean build_launchers (gpointer data);
static gboolean blacklist_init (gpointer data);
static gboolean blacklist_add (gpointer data);
static gboolean blacklist_remove (gpointer data);
static void blacklist_dir_changed (GFileMonitor * monitor, GFile * file, GFile * other_file, GFileMonitorEvent event_type, gpointer user_data);
static void app_dir_changed (GFileMonitor * monitor, GFile * file, GFile * other_file, GFileMonitorEvent event_type, gpointer user_data);
static gboolean destroy_launcher (gpointer data);


/*
 * Server List
 */

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

/*
 * Item List
 */

typedef struct _imList_t imList_t;
struct _imList_t {
	IndicateListenerServer * server;
	IndicateListenerIndicator * indicator;
	DbusmenuMenuitem * menuitem;
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

/*
 * Launcher List
 */

typedef struct _launcherList_t launcherList_t;
struct _launcherList_t {
	LauncherMenuItem * menuitem;
	GList * appdiritems;
};

static gint
launcherList_sort (gconstpointer a, gconstpointer b)
{
	launcherList_t * pa, * pb;

	pa = (launcherList_t *)a;
	pb = (launcherList_t *)b;

	const gchar * pan = launcher_menu_item_get_name(pa->menuitem);
	const gchar * pbn = launcher_menu_item_get_name(pb->menuitem);

	return g_strcmp0(pan, pbn);
}

/*
 * Black List
 */

static GHashTable * blacklist = NULL;
static GFileMonitor * blacklistdirmon = NULL;

/* Initialize the black list and start to setup
   handlers for it. */
static gboolean
blacklist_init (gpointer data)
{
	blacklist = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                  g_free, g_free);

	gchar * blacklistdir = g_build_filename(g_get_user_config_dir(), USER_BLACKLIST_DIR, NULL);
	g_debug("Looking at blacklist: %s", blacklistdir);
	if (!g_file_test(blacklistdir, G_FILE_TEST_IS_DIR)) {
		g_free(blacklistdir);
		return FALSE;
	}

	GFile * filedir = g_file_new_for_path(blacklistdir);
	blacklistdirmon = g_file_monitor_directory(filedir, G_FILE_MONITOR_NONE, NULL, NULL);
	if (blacklistdirmon != NULL) {
		g_signal_connect(G_OBJECT(blacklistdirmon), "changed", G_CALLBACK(blacklist_dir_changed), NULL);
	}

	GError * error = NULL;
	GDir * dir = g_dir_open(blacklistdir, 0, &error);
	if (dir == NULL) {
		g_warning("Unable to open blacklist directory (%s): %s", blacklistdir, error->message);
		g_error_free(error);
		g_free(blacklistdir);
		return FALSE;
	}

	const gchar * filename = NULL;
	while ((filename = g_dir_read_name(dir)) != NULL) {
		g_debug("Found file: %s", filename);
		gchar * path = g_build_filename(blacklistdir, filename, NULL);
		g_idle_add(blacklist_add, path);
	}

	g_dir_close(dir);
	g_free(blacklistdir);

	return FALSE;
}

/* Add a definition file into the black list and eclipse
   and launchers that have the same file. */
static gboolean
blacklist_add (gpointer udata)
{
	gchar * definition_file = (gchar *)udata;
	/* Dump the file */
	gchar * desktop;
	g_file_get_contents(definition_file, &desktop, NULL, NULL);
	if (desktop == NULL) {
		g_warning("Couldn't get data out of: %s", definition_file);
		return FALSE;
	}

	/* Clean up the data */
	gchar * trimdesktop = pango_trim_string(desktop);
	g_free(desktop);

	/* Check for conflicts */
	gpointer data = g_hash_table_lookup(blacklist, trimdesktop);
	if (data != NULL) {
		gchar * oldfile = (gchar *)data;
		if (!g_strcmp0(oldfile, definition_file)) {
			g_warning("Already added file '%s'", oldfile);
		} else {
			g_warning("Already have desktop file '%s' in blacklist file '%s' not adding from '%s'", trimdesktop, oldfile, definition_file);
		}

		g_free(trimdesktop);
		g_free(definition_file);
		return FALSE;
	}

	/* Actually blacklist this thing */
	g_hash_table_insert(blacklist, trimdesktop, definition_file);
	g_debug("Adding Blacklist item '%s' for desktop '%s'", definition_file, trimdesktop);

	/* Go through and eclipse folks */
	GList * launcher;
	for (launcher = launcherList; launcher != NULL; launcher = launcher->next) {
		launcherList_t * item = (launcherList_t *)launcher->data;
		if (!g_strcmp0(trimdesktop, launcher_menu_item_get_desktop(item->menuitem))) {
			launcher_menu_item_set_eclipsed(item->menuitem, TRUE);
		}
	}

	return FALSE;
}

/* Remove a black list item based on the definition file
   and uneclipse those launchers blocked by it. */
static gboolean
blacklist_remove (gpointer data)
{
	gchar * definition_file = (gchar *)data;
	g_debug("Removing: %s", definition_file);

	GHashTableIter iter;
	gpointer key, value;
	gboolean found = FALSE;

	g_hash_table_iter_init(&iter, blacklist);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		if (!g_strcmp0((gchar *)value, definition_file)) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		g_debug("\tNot found!");
		return FALSE;
	}

	GList * launcheritem;
	for (launcheritem = launcherList; launcheritem != NULL; launcheritem = launcheritem->next) {
		launcherList_t * li = (launcherList_t *)launcheritem->data;
		if (!g_strcmp0(launcher_menu_item_get_desktop(li->menuitem), (gchar *)key)) {
			GList * serveritem;
			for (serveritem = serverList; serveritem != NULL; serveritem = serveritem->next) {
				serverList_t * si = (serverList_t *)serveritem->data;
				if (!g_strcmp0(app_menu_item_get_desktop(si->menuitem), (gchar *)key)) {
					break;
				}
			}
			if (serveritem == NULL) {
				launcher_menu_item_set_eclipsed(li->menuitem, FALSE);
			}
		}
	}

	if (!g_hash_table_remove(blacklist, key)) {
		g_warning("Unable to remove '%s' with value '%s'", definition_file, (gchar *)key);
	}

	return FALSE;
}

/* Check to see if a particular desktop file is
   in the blacklist. */
static gboolean
blacklist_check (const gchar * desktop_file)
{
	g_debug("Checking blacklist for: %s", desktop_file);
	if (blacklist == NULL) return FALSE;

	if (g_hash_table_lookup(blacklist, desktop_file)) {
		g_debug("\tFound!");
		return TRUE;
	}

	return FALSE;
}

/* A callback everytime the blacklist directory changes
   in some way.  It needs to handle that. */
static void
blacklist_dir_changed (GFileMonitor * monitor, GFile * file, GFile * other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	g_debug("Blacklist directory changed!");

	switch (event_type) {
	case G_FILE_MONITOR_EVENT_DELETED: {
		gchar * path = g_file_get_path(file);
		g_debug("\tDelete: %s", path);
		g_idle_add(blacklist_remove, path);
		break;
	}
	case G_FILE_MONITOR_EVENT_CREATED: {
		gchar * path = g_file_get_path(file);
		g_debug("\tCreate: %s", path);
		g_idle_add(blacklist_add, path);
		break;
	}
	default:
		break;
	}

	return;
}

/*
 * More code
 */

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

	DbusmenuMenuitem * menushell = DBUSMENU_MENUITEM(data);
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

	dbusmenu_menuitem_child_append(menushell, DBUSMENU_MENUITEM(menuitem));
	/* Should be prepend ^ */

	resort_menu(menushell);

	return;
}

static void
server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data)
{
	serverList = g_list_sort(serverList, serverList_sort);
	check_eclipses(appitem);
	resort_menu(DBUSMENU_MENUITEM(data));
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
		/* gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages-new", DESIGN_TEAM_SIZE); */
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
		/* gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages", DESIGN_TEAM_SIZE); */
	}

	return;
}

static void
im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data)
{
	serverList_t * sl = (serverList_t *)data;
	sl->imList = g_list_sort(sl->imList, imList_sort);
	resort_menu(root_menuitem);
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

	remove_eclipses(sltp->menuitem);

	while (sltp->imList) {
		imList_t * imitem = (imList_t *)sltp->imList->data;
		indicator_removed(listener, server, imitem->indicator, "message", data);
	}

	serverList = g_list_remove(serverList, sltp);

	if (sltp->menuitem != NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(sltp->menuitem), "visibile", "false");
		dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(data), DBUSMENU_MENUITEM(sltp->menuitem));
		g_object_unref(G_OBJECT(sltp->menuitem));
	}

	g_free(sltp);

	/* Simulate a server saying zero to recalculate icon */
	server_count_changed(NULL, 0, NULL);

	return;
}

typedef struct _menushell_location menushell_location_t;
struct _menushell_location {
	const IndicateListenerServer * server;
	gint position;
	gboolean found;
};

static void
menushell_foreach_cb (DbusmenuMenuitem * data_mi, gpointer data_ms) {
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
resort_menu (DbusmenuMenuitem * menushell)
{
	guint position = 0;
	GList * serverentry;
	GList * launcherentry = launcherList;

	g_debug("Reordering Menu:");

	for (serverentry = serverList; serverentry != NULL; serverentry = serverentry->next) {
		serverList_t * si = (serverList_t *)serverentry->data;
		
		if (launcherentry != NULL) {
			launcherList_t * li = (launcherList_t *)launcherentry->data;
			while (launcherentry != NULL && g_strcmp0(launcher_menu_item_get_name(li->menuitem), app_menu_item_get_name(si->menuitem)) < 0) {
				g_debug("\tMoving launcher '%s' to position %d", launcher_menu_item_get_name(li->menuitem), position);
				dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->menuitem), position);

				position++;
				launcherentry = launcherentry->next;
				if (launcherentry != NULL) {
					li = (launcherList_t *)launcherentry->data;
				}
			}
		}

		if (si->menuitem != NULL) {
			g_debug("\tMoving app %s to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(si->server), position);
			dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(si->menuitem), position);
			position++;
		}

		GList * imentry;
		for (imentry = si->imList; imentry != NULL; imentry = imentry->next) {
			imList_t * imi = (imList_t *)imentry->data;

			if (imi->menuitem != NULL) {
				g_debug("\tMoving indicator on %s id %d to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(imi->server), INDICATE_LISTENER_INDICATOR_ID(imi->indicator), position);
				dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(imi->menuitem), position);
				position++;
			}
		}
	}

	while (launcherentry != NULL) {
		launcherList_t * li = (launcherList_t *)launcherentry->data;
		g_debug("\tMoving launcher '%s' to position %d", launcher_menu_item_get_name(li->menuitem), position);
		dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->menuitem), position);

		position++;
		launcherentry = launcherentry->next;
	}

	return;
}

static void
subtype_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	DbusmenuMenuitem * menushell = DBUSMENU_MENUITEM(data);
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
		listItem->menuitem = DBUSMENU_MENUITEM(menuitem);

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

		dbusmenu_menuitem_foreach(DBUSMENU_MENUITEM(menushell), menushell_foreach_cb, &msl);
		if (msl.found) {
			dbusmenu_menuitem_child_add_position(menushell, DBUSMENU_MENUITEM(menuitem), msl.position);
		} else {
			g_warning("Unable to find server menu item");
			dbusmenu_menuitem_child_append(menushell, DBUSMENU_MENUITEM(menuitem));
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
	DbusmenuMenuitem * menuitem = NULL;
	imList_t * ilt = NULL;
	if (listItem != NULL) {
		ilt = (imList_t *)listItem->data;
		menuitem = ilt->menuitem;
	}

	if (!removed && menuitem != NULL) {
		sl_item->imList = g_list_remove(sl_item->imList, ilt);
		g_signal_handler_disconnect(menuitem, ilt->timechange_cb);
		g_free(ilt);

		dbusmenu_menuitem_property_set(menuitem, "visibile", "false");
		dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(data), menuitem);
		removed = TRUE;
	}

	if (!removed) {
		g_warning("We were asked to remove %s %d but we didn't.", INDICATE_LISTENER_SERVER_DBUS_NAME(server), INDICATE_LISTENER_INDICATOR_ID(indicator));
	}

	return;
}

static void
app_dir_changed (GFileMonitor * monitor, GFile * file, GFile * other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	gchar * directory = (gchar *)user_data;
	g_debug("Application directory changed: %s", directory);

	switch (event_type) {
	case G_FILE_MONITOR_EVENT_DELETED: {
		gchar * path = g_file_get_path(file);
		g_debug("\tDelete: %s", path);
		g_idle_add(destroy_launcher, path);
		break;
	}
	case G_FILE_MONITOR_EVENT_CREATED: {
		gchar * path = g_file_get_path(file);
		g_debug("\tCreate: %s", path);
		g_idle_add(build_launcher, path);
		break;
	}
	default:
		break;
	}

	return;
}

/* Check to see if a new desktop file causes
   any of the launchers to be eclipsed by a running
   process */
static void
check_eclipses (AppMenuItem * ai)
{
	g_debug("Checking eclipsing");
	const gchar * aidesktop = app_menu_item_get_desktop(ai);
	if (aidesktop == NULL) return;
	g_debug("\tApp desktop: %s", aidesktop);

	GList * llitem;
	for (llitem = launcherList; llitem != NULL; llitem = llitem->next) {
		launcherList_t * ll = (launcherList_t *)llitem->data;
		const gchar * lidesktop = launcher_menu_item_get_desktop(ll->menuitem);
		g_debug("\tLauncher desktop: %s", lidesktop);

		if (!g_strcmp0(aidesktop, lidesktop)) {
			launcher_menu_item_set_eclipsed(ll->menuitem, TRUE);
			break;
		}
	}

	return;
}

/* Remove any eclipses that might have been caused
   by this app item that is now retiring */
static void
remove_eclipses (AppMenuItem * ai)
{
	const gchar * aidesktop = app_menu_item_get_desktop(ai);
	if (aidesktop == NULL) return;

	if (blacklist_check(aidesktop)) return;

	GList * llitem;
	for (llitem = launcherList; llitem != NULL; llitem = llitem->next) {
		launcherList_t * ll = (launcherList_t *)llitem->data;
		const gchar * lidesktop = launcher_menu_item_get_desktop(ll->menuitem);

		if (!g_strcmp0(aidesktop, lidesktop)) {
			launcher_menu_item_set_eclipsed(ll->menuitem, FALSE);
			break;
		}
	}

	return;
}

/* Remove a launcher from the system.  We need to figure
   out what it's up to! */
static gboolean
destroy_launcher (gpointer data)
{
	gchar * appdirentry = (gchar *)data;

	GList * listitem;
	GList * direntry;
	launcherList_t * li;
	gchar * appdir;

	for (listitem = launcherList; listitem != NULL; listitem = listitem->next) {
		li = (launcherList_t *)listitem->data;
		for (direntry = li->appdiritems; direntry != NULL; direntry = direntry->next) {
			appdir = (gchar *)direntry->data;
			if (!g_strcmp0(appdir, appdirentry)) {
				break;
			}
		}

		if (direntry != NULL) {
			break;
		}
	}

	if (listitem == NULL) {
		g_warning("Removed '%s' by the way of it not seeming to exist anywhere.", appdirentry);
		return FALSE;
	}

	if (g_list_length(li->appdiritems) > 1) {
		/* Just remove this item, and we can move on */
		g_debug("Just removing file entry: %s", appdir);
		li->appdiritems = g_list_remove(li->appdiritems, appdir);
		g_free(appdir);
		return FALSE;
	}

	/* Full Destroy */

	return FALSE;
}

/* This function turns a specific file into a menu
   item and registers it appropriately with everyone */
static gboolean
build_launcher (gpointer data)
{
	/* Read the file get the data */
	gchar * path = (gchar *)data;
	g_debug("\tpath: %s", path);
	gchar * desktop = NULL;
	
	g_file_get_contents(path, &desktop, NULL, NULL);

	if (desktop == NULL) {
		return FALSE;
	}

	gchar * trimdesktop = pango_trim_string(desktop);
	g_free(desktop);
	g_debug("\tcontents: %s", trimdesktop);

	/* Check to see if we already have a launcher */
	GList * listitem;
	for (listitem = launcherList; listitem != NULL; listitem = listitem->next) {
		launcherList_t * li = (launcherList_t *)listitem->data;
		if (!g_strcmp0(launcher_menu_item_get_desktop(li->menuitem), trimdesktop)) {
			break;
		}
	}

	if (listitem == NULL) {
		/* If not */
		/* Build the item */
		launcherList_t * ll = g_new0(launcherList_t, 1);
		ll->menuitem = launcher_menu_item_new(trimdesktop);
		g_free(trimdesktop);
		ll->appdiritems = g_list_append(NULL, path);

		/* Add it to the list */
		launcherList = g_list_insert_sorted(launcherList, ll, launcherList_sort);

		/* Add it to the menu */
		dbusmenu_menuitem_child_append(root_menuitem, DBUSMENU_MENUITEM(ll->menuitem));
		resort_menu(root_menuitem);

		if (blacklist_check(launcher_menu_item_get_desktop(ll->menuitem))) {
			launcher_menu_item_set_eclipsed(ll->menuitem, TRUE);
		}
	} else {
		/* If so add ourselves */
		launcherList_t * ll = (launcherList_t *)listitem->data;
		ll->appdiritems = g_list_append(ll->appdiritems, path);
	}

	return FALSE;
}

/* This function goes through all the launchers that we're
   supposed to be grabbing and decides to show turn them
   into menu items or not.  It doens't do the work, but it
   makes the decision. */
static gboolean
build_launchers (gpointer data)
{
	gchar * directory = (gchar *)data;

	if (!g_file_test(directory, G_FILE_TEST_IS_DIR)) {
		return FALSE;
	}

	GFile * filedir = g_file_new_for_path(directory);
	GFileMonitor * dirmon = g_file_monitor_directory(filedir, G_FILE_MONITOR_NONE, NULL, NULL);
	if (dirmon != NULL) {
		g_signal_connect(G_OBJECT(dirmon), "changed", G_CALLBACK(app_dir_changed), directory);
	}

	GError * error = NULL;
	GDir * dir = g_dir_open(directory, 0, &error);
	if (dir == NULL) {
		g_warning("Unable to open system apps directory: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	const gchar * filename = NULL;
	while ((filename = g_dir_read_name(dir)) != NULL) {
		g_debug("Found file: %s", filename);
		gchar * path = g_build_filename(directory, filename, NULL);
		g_idle_add(build_launcher, path);
	}

	g_dir_close(dir);
	launcherList = g_list_sort(launcherList, launcherList_sort);
	return FALSE;
}

/* Oh, if you don't know what main() is for
   we really shouldn't be talking. */
int
main (int argc, char ** argv)
{
	g_type_init();

	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	DBusGProxy * bus_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	GError * error = NULL;
	guint nameret = 0;

	if (!org_freedesktop_DBus_request_name(bus_proxy, INDICATOR_MESSAGES_DBUS_NAME, 0, &nameret, &error)) {
		g_error("Unable to call to request name");
		return 1;
	}

	if (nameret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_error("Unable to get name");
		return 1;
	}

	listener = indicate_listener_ref_default();
	serverList = NULL;

	root_menuitem = dbusmenu_menuitem_new();
	DbusmenuServer * server = dbusmenu_server_new(INDICATOR_MESSAGES_DBUS_OBJECT);
	dbusmenu_server_set_root(server, root_menuitem);

	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_INDICATOR_ADDED, G_CALLBACK(indicator_added), root_menuitem);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_INDICATOR_REMOVED, G_CALLBACK(indicator_removed), root_menuitem);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_SERVER_ADDED, G_CALLBACK(server_added), root_menuitem);
	g_signal_connect(listener, INDICATE_LISTENER_SIGNAL_SERVER_REMOVED, G_CALLBACK(server_removed), root_menuitem);

	g_idle_add(blacklist_init, NULL);
	g_idle_add(build_launchers, SYSTEM_APPS_DIR);
	gchar * userdir = g_build_filename(g_get_user_config_dir(), USER_APPS_DIR, NULL);
	g_idle_add(build_launchers, userdir);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_free(userdir);

	return 0;
}
