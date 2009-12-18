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
#include <locale.h>
#include <libintl.h>
#include <config.h>
#include <pango/pango-utils.h>
#include <dbus/dbus-glib-bindings.h>
#include <libindicate/listener.h>
#include <gio/gio.h>

#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/server.h>

#include "im-menu-item.h"
#include "app-menu-item.h"
#include "launcher-menu-item.h"
#include "dbus-data.h"
#include "dirs.h"
#include "messages-service-dbus.h"

static IndicateListener * listener;
static GList * serverList = NULL;
static GList * launcherList = NULL;

static DbusmenuMenuitem * root_menuitem = NULL;
static GMainLoop * mainloop = NULL;

static MessageServiceDbus * dbus_interface = NULL;


static void server_count_changed (AppMenuItem * appitem, guint count, gpointer data);
static void server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data);
static void im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data);
static void resort_menu (DbusmenuMenuitem * menushell);
static void indicator_removed (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gpointer data);
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
static void check_hidden (void);


/*
 * Server List
 */

typedef struct _serverList_t serverList_t;
struct _serverList_t {
	IndicateListenerServer * server;
	AppMenuItem * menuitem;
	DbusmenuMenuitem * separator;
	gboolean attention;
	guint count;
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
	gulong attentionchange_cb;
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
	DbusmenuMenuitem * separator;
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

static void
launcherList_count_helper (gpointer data, gpointer user_data)
{
	guint * count = (guint *)user_data;
	launcherList_t * li = (launcherList_t *)data;

	if (!launcher_menu_item_get_eclipsed(li->menuitem)) {
		*count = *count + 1;
	}

	return;
}

static guint
launcherList_count (void)
{
	guint count = 0;

	g_list_foreach(launcherList, launcherList_count_helper, &count);

	return count;
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
		g_warning("Unable to open blacklist directory (%s): %s", blacklistdir, error == NULL ? "No Message" : error->message);
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
			dbusmenu_menuitem_property_set(item->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
		}
	}

	check_hidden();

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
				dbusmenu_menuitem_property_set(li->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "true");
			}
		}
	}

	if (!g_hash_table_remove(blacklist, key)) {
		g_warning("Unable to remove '%s' with value '%s'", definition_file, (gchar *)key);
	}

	check_hidden();
	resort_menu(root_menuitem);

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

/* Goes through all the servers and sees if any of them
   want attention.  If they do, then well we'll give it
   to them.  If they don't, let's not bother the user
   any, shall we? */
static void
check_attention (void)
{
	GList * pointer;
	for (pointer = serverList; pointer != NULL; pointer = g_list_next(pointer)) {
		serverList_t * slt = (serverList_t *)pointer->data;
		if (slt->attention) {
			message_service_dbus_set_attention(dbus_interface, TRUE);
			return;
		}
	}
	message_service_dbus_set_attention(dbus_interface, FALSE);
	return;
}

/* This checks a server listing to see if it should
   have attention.  It can get attention through it's
   count or by having an indicator that is requestion
   attention. */
static void
server_attention (serverList_t * slt)
{
	/* Count, easy yes and out. */
	if (slt->count > 0) {
		slt->attention = TRUE;
		return;
	}

	/* Check to see if any of the indicators want attention */
	GList * pointer;
	for (pointer = slt->imList; pointer != NULL; pointer = g_list_next(pointer)) {
		imList_t * ilt = (imList_t *)pointer->data;
		if (im_menu_item_get_attention(IM_MENU_ITEM(ilt->menuitem))) {
			slt->attention = TRUE;
			return;
		}
	}

	/* Nope, no one */
	slt->attention = FALSE;
	return;
}

/* A new server has been created on the indicate bus.
   We need to check to see if we like it.  And build
   structures for it if so. */
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

	/* Build the Menu item */
	AppMenuItem * menuitem = app_menu_item_new(listener, server);

	/* Build a possible server structure */
	serverList_t * sl_item = g_new0(serverList_t, 1);
	sl_item->server = server;
	sl_item->menuitem = menuitem;
	sl_item->imList = NULL;
	sl_item->attention = FALSE;
	sl_item->count = 0;

	/* Build a separator */
	sl_item->separator = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(sl_item->separator, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);

	/* Incase we got an indicator first */
	GList * alreadythere = g_list_find_custom(serverList, sl_item, serverList_equal);
	if (alreadythere != NULL) {
		/* Use the one we already had */
		g_free(sl_item);
		sl_item = (serverList_t *)alreadythere->data;
		sl_item->menuitem = menuitem;
		serverList = g_list_sort(serverList, serverList_sort);
	} else {
		/* Insert the new one in the list */
		serverList = g_list_insert_sorted(serverList, sl_item, serverList_sort);
	}

	/* Connect the signals up to the menu item */
	g_signal_connect(G_OBJECT(menuitem), APP_MENU_ITEM_SIGNAL_COUNT_CHANGED, G_CALLBACK(server_count_changed), sl_item);
	g_signal_connect(G_OBJECT(menuitem), APP_MENU_ITEM_SIGNAL_NAME_CHANGED,  G_CALLBACK(server_name_changed),  menushell);

	/* Put our new menu item in, with the separator behind it.
	   resort_menu will take care of whether it should be hidden
	   or not. */
	dbusmenu_menuitem_child_append(menushell, DBUSMENU_MENUITEM(menuitem));
	dbusmenu_menuitem_child_append(menushell, DBUSMENU_MENUITEM(sl_item->separator));

	resort_menu(menushell);
	check_hidden();

	return;
}

/* The name of a server has changed, we probably
   need to reorder the menu to keep it in alphabetical
   order.  This happens often after we read the destkop
   file from disk. */
static void
server_name_changed (AppMenuItem * appitem, gchar * name, gpointer data)
{
	serverList = g_list_sort(serverList, serverList_sort);
	check_eclipses(appitem);
	resort_menu(DBUSMENU_MENUITEM(data));
	return;
}

/* If the count on the server changes, we need to know
   whether that should be grabbing attention or not.  If
   it is, we need to reevaluate whether the whole thing
   should be grabbing attention or not. */
static void
server_count_changed (AppMenuItem * appitem, guint count, gpointer data)
{
	serverList_t * slt = (serverList_t *)data;
	slt->count = count;

	if (count == 0 && slt->attention) {
		/* Regen based on indicators if the count isn't going to cause it. */
		server_attention(slt);
		/* If we're dropping let's see if we're the last. */
		if (!slt->attention) {
			check_attention();
		}
	}

	if (count != 0 && !slt->attention) {
		slt->attention = TRUE;
		/* Let's tell everyone about us! */
		message_service_dbus_set_attention(dbus_interface, TRUE);
	}

	return;
}

/* Respond to the IM entrie's time changing
   which results in it needing to resort the list
   and rebuild the menu to match. */
static void
im_time_changed (ImMenuItem * imitem, glong seconds, gpointer data)
{
	serverList_t * sl = (serverList_t *)data;
	sl->imList = g_list_sort(sl->imList, imList_sort);
	resort_menu(root_menuitem);
	return;
}

/* The IM entrie's request for attention has changed
   so we need to pass that up the stack. */
static void
im_attention_changed (ImMenuItem * imitem, gboolean requestit, gpointer data)
{
	serverList_t * sl = (serverList_t *)data;

	if (requestit) {
		sl->attention = TRUE;
		message_service_dbus_set_attention(dbus_interface, TRUE);
	} else {
		server_attention(sl);
		if (!sl->attention) {
			check_attention();
		}
	}

	return;
}

/* Run when a server is removed from the indicator bus.  It figures
   out if we have it somewhere, and if so then we dump it out and
   clean up all of it's entries. */
static void 
server_removed (IndicateListener * listener, IndicateListenerServer * server, gchar * type, gpointer data)
{
	/* Look for the server */
	g_debug("Removing server: %s", INDICATE_LISTENER_SERVER_DBUS_NAME(server));
	serverList_t slt = {0};
	slt.server = server;
	GList * lookup = g_list_find_custom(serverList, &slt, serverList_equal);

	/* If we don't have it, exit */
	if (lookup == NULL) {
		g_debug("\tUnable to find server: %s", INDICATE_LISTENER_SERVER_DBUS_NAME(server));
		return;
	}

	serverList_t * sltp = (serverList_t *)lookup->data;

	/* Removing indicators from this server */
	while (sltp->imList) {
		imList_t * imitem = (imList_t *)sltp->imList->data;
		indicator_removed(listener, server, imitem->indicator, data);
	}

	/* Remove from the server list */
	serverList = g_list_remove(serverList, sltp);

	/* Remove launchers this could be eclipsing */
	remove_eclipses(sltp->menuitem);

	/* If there is a menu item, let's get rid of it. */
	if (sltp->menuitem != NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(sltp->menuitem), DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
		dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(data), DBUSMENU_MENUITEM(sltp->menuitem));
		g_object_unref(G_OBJECT(sltp->menuitem));
	}

	/* If there is a separator, let's get rid of it. */
	if (sltp->separator != NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(sltp->separator), DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
		dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(data), DBUSMENU_MENUITEM(sltp->separator));
		g_object_unref(G_OBJECT(sltp->separator));
	}

	if (sltp->attention) {
		/* Check to see if this was the server causing the menu item to
		   be lit up. */
		check_attention();
	}

	g_free(sltp);

	check_hidden();

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
check_hidden (void)
{
	g_debug("Checking Hidden...");
	gboolean hide = FALSE;
	if (launcherList_count() == 0) {
		g_debug("\tZero Launchers");
		/* If we don't have visible launchers we need to look more */
		if (g_list_length(serverList) == 0) {
			g_debug("\tZero Applications");
			hide = TRUE;	
		}
	}

	message_service_dbus_set_icon(dbus_interface, hide);
	return;
}

/* This function takes care of putting the menu in the right order.
   It basically it rebuilds the order by looking through all the
   applications and launchers and puts them in the right place.  The
   menu functions will handle the cases where they don't move so this
   is a good way to ensure everything is right. */
static void
resort_menu (DbusmenuMenuitem * menushell)
{
	guint position = 0;
	GList * serverentry;
	GList * launcherentry = launcherList;
	DbusmenuMenuitem * last_separator = NULL;

	g_debug("Reordering Menu:");

	for (serverentry = serverList; serverentry != NULL; serverentry = serverentry->next) {
		serverList_t * si = (serverList_t *)serverentry->data;
		
		/* Looking to see if there are any launchers we need to insert
		   into the menu structure.  We put as many as we need to. */
		if (launcherentry != NULL) {
			launcherList_t * li = (launcherList_t *)launcherentry->data;
			while (launcherentry != NULL && g_strcmp0(launcher_menu_item_get_name(li->menuitem), app_menu_item_get_name(si->menuitem)) < 0) {
				/* Putting the launcher item in */
				g_debug("\tMoving launcher '%s' to position %d", launcher_menu_item_get_name(li->menuitem), position);
				dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->menuitem), position);
				position++;

				/* Putting the launcher separator in */
				g_debug("\tMoving launcher separator to position %d", position);
				dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->separator), position);
				if (!launcher_menu_item_get_eclipsed(li->menuitem)) {
					/* Only clear the visiblity if we're not eclipsed */
					dbusmenu_menuitem_property_set(li->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "true");
					last_separator = li->separator;
				}
				position++;

				launcherentry = launcherentry->next;
				if (launcherentry != NULL) {
					li = (launcherList_t *)launcherentry->data;
				}
			}
		}

		/* Putting the app menu item in */
		if (si->menuitem != NULL) {
			g_debug("\tMoving app %s to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(si->server), position);
			dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(si->menuitem), position);
			position++;
		}

		/* Putting all the indicators that are related to this application
		   after it. */
		GList * imentry;
		for (imentry = si->imList; imentry != NULL; imentry = imentry->next) {
			imList_t * imi = (imList_t *)imentry->data;

			if (imi->menuitem != NULL) {
				g_debug("\tMoving indicator on %s id %d to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(imi->server), INDICATE_LISTENER_INDICATOR_ID(imi->indicator), position);
				dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(imi->menuitem), position);
				position++;
			}
		}

		/* Lastly putting the separator in */
		if (si->separator != NULL) {
			g_debug("\tMoving app %s separator to position %d", INDICATE_LISTENER_SERVER_DBUS_NAME(si->server), position);
			dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(si->separator), position);
			dbusmenu_menuitem_property_set(si->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "true");
			position++;
			last_separator = si->separator;
		}
	}

	/* Put any leftover launchers in at the end of the list. */
	while (launcherentry != NULL) {
		launcherList_t * li = (launcherList_t *)launcherentry->data;

		/* Putting the launcher in */
		g_debug("\tMoving launcher '%s' to position %d", launcher_menu_item_get_name(li->menuitem), position);
		dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->menuitem), position);
		position++;

		/* Putting the launcher separator in */
		g_debug("\tMoving launcher separator to position %d", position);
		dbusmenu_menuitem_child_reorder(DBUSMENU_MENUITEM(menushell), DBUSMENU_MENUITEM(li->separator), position);
		if (!launcher_menu_item_get_eclipsed(li->menuitem)) {
			/* Only clear the visiblity if we're not eclipsed */
			dbusmenu_menuitem_property_set(li->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "true");
			last_separator = li->separator;
		}
		position++;

		launcherentry = launcherentry->next;
	}

	if (last_separator != NULL) {
		dbusmenu_menuitem_property_set(last_separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
	} else {
		g_warning("No last separator on resort");
	}

	return;
}

/* Responding to a new indicator showing up on the bus.  We
   need to create a menuitem for it and start populating the
   internal structures to track it. */
static void
indicator_added (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gpointer data)
{
	DbusmenuMenuitem * menushell = DBUSMENU_MENUITEM(data);
	if (menushell == NULL) {
		g_error("Data in callback is not a menushell");
		return;
	}

	imList_t * listItem = g_new0(imList_t, 1);
	listItem->server = server;
	listItem->indicator = indicator;

	/* Building the IM Menu Item which is a subclass
	   of DBus Menuitem */
	ImMenuItem * menuitem = im_menu_item_new(listener, server, indicator);
	g_object_ref(G_OBJECT(menuitem));
	listItem->menuitem = DBUSMENU_MENUITEM(menuitem);

	/* Looking for a server entry to attach this indicator
	   to.  If we can't find one then we have to build one
	   and attach the indicator to it. */
	serverList_t sl_item_local = {0};
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
		sl_item->attention = FALSE;
		sl_item->count = 0;
		sl_item->separator = NULL;

		serverList = g_list_insert_sorted(serverList, sl_item, serverList_sort);
	} else {
		sl_item = (serverList_t *)serverentry->data;
	}

	/* Added a this entry into the IM list */
	sl_item->imList = g_list_insert_sorted(sl_item->imList, listItem, imList_sort);
	listItem->timechange_cb = g_signal_connect(G_OBJECT(menuitem), IM_MENU_ITEM_SIGNAL_TIME_CHANGED, G_CALLBACK(im_time_changed), sl_item);
	listItem->attentionchange_cb = g_signal_connect(G_OBJECT(menuitem), IM_MENU_ITEM_SIGNAL_ATTENTION_CHANGED, G_CALLBACK(im_attention_changed), sl_item);

	/* Check the length of the list.  If we've got more inidactors
	   than we allow.  Well.  Someone's gotta pay.  Sorry.  I didn't
	   want to do this, but you did it to yourself. */
	if (g_list_length(sl_item->imList) > MAX_NUMBER_OF_INDICATORS) {
		GList * indicatoritem;
		gint count;
		for (indicatoritem = sl_item->imList, count = 0; indicatoritem != NULL; indicatoritem = g_list_next(indicatoritem), count++) {
			imList_t * im = (imList_t *)indicatoritem->data;
			im_menu_item_show(IM_MENU_ITEM(im->menuitem), count < MAX_NUMBER_OF_INDICATORS);
		}
	}

	/* Placing the item into the shell.  Look to see if
	   we can find our server and slip in there.  Otherwise
	   we'll just append. */
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

	return;
}

/* Process and indicator getting removed from the system.  We
   first need to ensure that it's one of ours and figure out
   where we put it.  When we find all that out we can go through
   the process of removing the effect it had on the system. */
static void
indicator_removed (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gpointer data)
{
	g_debug("Removing %s %d", INDICATE_LISTENER_SERVER_DBUS_NAME(server), INDICATE_LISTENER_INDICATOR_ID(indicator));

	gboolean removed = FALSE;

	/* Find the server that was related to this item */
	serverList_t sl_item_local = {0};
	serverList_t * sl_item = NULL;
	sl_item_local.server = server;
	GList * serverentry = g_list_find_custom(serverList, &sl_item_local, serverList_equal);
	if (serverentry == NULL) {
		/* We didn't care about that server */
		return;
	}
	sl_item = (serverList_t *)serverentry->data;

	/* Look in the IM Hash Table */
	imList_t listData = {0};
	listData.server = server;
	listData.indicator = indicator;

	GList * listItem = g_list_find_custom(sl_item->imList, &listData, imList_equal);
	DbusmenuMenuitem * menuitem = NULL;
	imList_t * ilt = NULL;
	if (listItem != NULL) {
		ilt = (imList_t *)listItem->data;
		menuitem = ilt->menuitem;
	}

	/* If we found a menu item and an imList_t item then
	   we can go ahead and remove it.  Otherwise we can 
	   skip this and exit. */
	if (!removed && menuitem != NULL) {
		sl_item->imList = g_list_remove(sl_item->imList, ilt);
		g_signal_handler_disconnect(menuitem, ilt->timechange_cb);
		g_signal_handler_disconnect(menuitem, ilt->attentionchange_cb);
		g_free(ilt);

		if (im_menu_item_get_attention(IM_MENU_ITEM(menuitem)) && im_menu_item_shown(IM_MENU_ITEM(menuitem))) {
			/* If the removed indicator menu item was asking for
			   attention we need to see if this server should still
			   be asking for attention. */
			server_attention(sl_item);
			/* If the server is no longer asking for attention then
			   we need to check if the whole system should be. */
			if (!sl_item->attention) {
				check_attention();
			}
		}

		if (im_menu_item_shown(IM_MENU_ITEM(menuitem)) && g_list_length(sl_item->imList) >= MAX_NUMBER_OF_INDICATORS) {
			/* In this case we need to show a different indicator
			   becasue a shown one has left.  But we're going to be
			   easy and set all the values. */
			GList * indicatoritem;
			gint count;
			for (indicatoritem = sl_item->imList, count = 0; indicatoritem != NULL; indicatoritem = g_list_next(indicatoritem), count++) {
				imList_t * im = (imList_t *)indicatoritem->data;
				im_menu_item_show(IM_MENU_ITEM(im->menuitem), count < MAX_NUMBER_OF_INDICATORS);
			}
		}

		/* Hide the item immediately, and then remove it
		   which might take a little longer. */
		dbusmenu_menuitem_property_set(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
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
			dbusmenu_menuitem_property_set(ll->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
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
			dbusmenu_menuitem_property_set(ll->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "true");
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
	g_free(appdir);
	g_list_free(li->appdiritems);

	if (li->menuitem != NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(li->menuitem), DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
		dbusmenu_menuitem_child_delete(root_menuitem, DBUSMENU_MENUITEM(li->menuitem));
		g_object_unref(G_OBJECT(li->menuitem));
		li->menuitem = NULL;
	}

	launcherList = g_list_remove(launcherList, li);
	g_free(li);

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

		/* Build a separator */
		ll->separator = dbusmenu_menuitem_new();
		dbusmenu_menuitem_property_set(ll->separator, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);

		/* Add it to the list */
		launcherList = g_list_insert_sorted(launcherList, ll, launcherList_sort);

		/* Add it to the menu */
		dbusmenu_menuitem_child_append(root_menuitem, DBUSMENU_MENUITEM(ll->menuitem));
		dbusmenu_menuitem_child_append(root_menuitem, DBUSMENU_MENUITEM(ll->separator));

		/* If we're in the black list or we've gotten eclipsed
		   by something else, hide the item and the separator. */
		if (blacklist_check(launcher_menu_item_get_desktop(ll->menuitem)) ||
				launcher_menu_item_get_eclipsed(ll->menuitem)) {
			launcher_menu_item_set_eclipsed(ll->menuitem, TRUE);
			dbusmenu_menuitem_property_set(ll->separator, DBUSMENU_MENUITEM_PROP_VISIBLE, "false");
		}

		resort_menu(root_menuitem);
		check_hidden();
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

	/* Setting up i18n and gettext.  Apparently, we need
	   all of these. */
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	dbus_interface = message_service_dbus_new();

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
	g_idle_add(build_launchers, SYSTEM_APPS_DIR_OLD);
	gchar * userdir = g_build_filename(g_get_user_config_dir(), USER_APPS_DIR, NULL);
	g_idle_add(build_launchers, userdir);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_free(userdir);

	return 0;
}
