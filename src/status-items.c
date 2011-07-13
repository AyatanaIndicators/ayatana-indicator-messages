
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libdbusmenu-glib/dbusmenu-glib.h>

#include "status-items.h"
#include "status-provider.h"

static const gchar * status_strings [STATUS_PROVIDER_STATUS_LAST] = {
  /* STATUS_PROVIDER_STATUS_ONLINE,    */   N_("Available"),
  /* STATUS_PROVIDER_STATUS_AWAY,      */   N_("Away"),
  /* STATUS_PROVIDER_STATUS_DND        */   N_("Busy"),
  /* STATUS_PROVIDER_STATUS_INVISIBLE  */   N_("Invisible"),
  /* STATUS_PROVIDER_STATUS_OFFLINE,   */   N_("Offline"),
  /* STATUS_PROVIDER_STATUS_DISCONNECTED*/  N_("Offline")
};

static const gchar * status_icons[STATUS_PROVIDER_STATUS_LAST] = {
  /* STATUS_PROVIDER_STATUS_ONLINE, */      "user-available",
  /* STATUS_PROVIDER_STATUS_AWAY, */        "user-away",
  /* STATUS_PROVIDER_STATUS_DND, */         "user-busy",
  /* STATUS_PROVIDER_STATUS_INVISIBLE, */   "user-invisible",
  /* STATUS_PROVIDER_STATUS_OFFLINE */      "user-offline",
  /* STATUS_PROVIDER_STATUS_DISCONNECTED */ "user-offline-panel"
};

static const gchar * panel_icons[STATUS_PROVIDER_STATUS_LAST] = {
  /* STATUS_PROVIDER_STATUS_ONLINE, */      "indicator-messages-user-available",
  /* STATUS_PROVIDER_STATUS_AWAY, */        "indicator-messages-user-away",
  /* STATUS_PROVIDER_STATUS_DND, */         "indicator-messages-user-busy",
  /* STATUS_PROVIDER_STATUS_INVISIBLE, */   "indicator-messages-user-invisible",
  /* STATUS_PROVIDER_STATUS_OFFLINE */      "indicator-messages-user-offline",
  /* STATUS_PROVIDER_STATUS_DISCONNECTED */ "indicator-messages-user-disconnected"
};

static const gchar * panel_active_icons[STATUS_PROVIDER_STATUS_LAST] = {
  /* STATUS_PROVIDER_STATUS_ONLINE, */      "indicator-messages-new-user-available",
  /* STATUS_PROVIDER_STATUS_AWAY, */        "indicator-messages-new-user-away",
  /* STATUS_PROVIDER_STATUS_DND, */         "indicator-messages-new-user-busy",
  /* STATUS_PROVIDER_STATUS_INVISIBLE, */   "indicator-messages-new-user-invisible",
  /* STATUS_PROVIDER_STATUS_OFFLINE */      "indicator-messages-new-user-offline",
  /* STATUS_PROVIDER_STATUS_DISCONNECTED */ "indicator-messages-new-user-disconnected"
};

/* Prototypes */
static gboolean provider_directory_parse (gpointer dir);
static gboolean load_status_provider (gpointer dir);

/* Globals */
static StatusProviderStatus current_status = STATUS_PROVIDER_STATUS_DISCONNECTED;
static GList * menuitems = NULL;
static GList * status_providers = NULL;
static StatusUpdateFunc update_func = NULL;

/* Build the inital status items and start kicking off the async code
   for handling all the statuses */
GList *
status_items_build (StatusUpdateFunc status_update_func)
{
	int i;
	for (i = STATUS_PROVIDER_STATUS_ONLINE; i < STATUS_PROVIDER_STATUS_DISCONNECTED; i++) {
		DbusmenuMenuitem * item = dbusmenu_menuitem_new();

		dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, _(status_strings[i]));
		dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_ICON_NAME, status_icons[i]);

		dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
		dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);

		dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
		dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

		menuitems = g_list_append(menuitems, item);
	}

	update_func = status_update_func;

	g_idle_add(provider_directory_parse, STATUS_PROVIDER_DIR);

	return menuitems;
}

/* Clean up our globals and stop with all this allocation
   of memory */
void
status_items_cleanup (void)
{
	while (status_providers != NULL) {
		StatusProvider * sprovider = STATUS_PROVIDER(status_providers->data);
		g_object_unref(sprovider);
		status_providers = g_list_remove(status_providers, sprovider);
	}

	return;
}

/* Get the icon that should be shown on the panel */
const gchar *
status_current_panel_icon (gboolean alert)
{
	if (alert) {
		return panel_active_icons[current_status];
	} else {
		return panel_icons[current_status];
	}
}

/* Update status from all the providers */
static void
update_status (void)
{
	StatusProviderStatus status = STATUS_PROVIDER_STATUS_DISCONNECTED;
	GList * provider;

	for (provider = status_providers; provider != NULL; provider = g_list_next(provider)) {
		StatusProviderStatus localstatus = status_provider_get_status(STATUS_PROVIDER(provider->data));

		if (localstatus < status) {
			status = localstatus;
		}
	}

	if (status == current_status) {
		return;
	}

	current_status = status;

	if (update_func != NULL) {
		update_func();
	}

	GList * menu;
	int i;
	for (menu = menuitems, i = 0; menu != NULL && i < STATUS_PROVIDER_STATUS_DISCONNECTED; menu = g_list_next(menu), i++) {
		/* If we're the seleced status or if we're disconnected
		   show the user that we're offline */
		if (i == current_status || (current_status == STATUS_PROVIDER_STATUS_DISCONNECTED && i == STATUS_PROVIDER_STATUS_OFFLINE)) {
			dbusmenu_menuitem_property_set_int(DBUSMENU_MENUITEM(menu->data), DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
		} else {
			dbusmenu_menuitem_property_set_int(DBUSMENU_MENUITEM(menu->data), DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
		}

		if (current_status == STATUS_PROVIDER_STATUS_DISCONNECTED) {
			dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(menu->data), DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);
		} else {
			dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(menu->data), DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
		}
	}

	return;
}

/* Start parsing a directory and setting up the entires in the idle loop */
static gboolean
provider_directory_parse (gpointer directory)
{
	const gchar * dirname = (const gchar *)directory;
	g_debug("Looking for status providers in: %s", dirname);

	if (!g_file_test(dirname, G_FILE_TEST_EXISTS)) {
		return FALSE;
	}

	GDir * dir = g_dir_open(dirname, 0, NULL);
	if (dir == NULL) {
		return FALSE;
	}

	const gchar * name;
	while ((name = g_dir_read_name(dir)) != NULL) {
		if (!g_str_has_suffix(name, G_MODULE_SUFFIX)) {
			continue;
		}

		gchar * fullname = g_build_filename(dirname, name, NULL);
		g_idle_add(load_status_provider, fullname);
	}

	g_dir_close(dir);

	return FALSE;
}

/* Close the module as an idle function so that we know
   it's all cleaned up */
static gboolean
module_destroy_in_idle_helper (gpointer data)
{
	GModule * module = (GModule *)data;
	if (module != NULL) {
		g_debug("Unloading module: %s", g_module_name(module));
		g_module_close(module);
	}
	return FALSE;
}

/* Set up an idle function to close the module */
static void
module_destroy_in_idle (gpointer data)
{
	g_idle_add_full(G_PRIORITY_LOW, module_destroy_in_idle_helper, data, NULL);
	return;
}

/* Load a particular status provider */
static gboolean
load_status_provider (gpointer dir)
{
	gchar * provider = (gchar *)dir;

	if (!g_file_test(provider, G_FILE_TEST_EXISTS)) {
		goto exit_final;
	}

	g_debug("Loading status provider: %s", provider);

	GModule * module;

	module = g_module_open(provider, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
	if (module == NULL) {
		g_warning("Unable to module for: %s", provider);
		goto exit_module_fail;
	}

	/* Got it */
	GType (*type_func) (void);
	if (!g_module_symbol(module, STATUS_PROVIDER_EXPORT_S, (gpointer *)&type_func)) {
		g_warning("Unable to find type symbol in: %s", provider);
		goto exit_module_fail;
	}

	GType provider_type = type_func();
	if (provider_type == 0) {
		g_warning("Unable to create type from: %s", provider);
		goto exit_module_fail;
	}

	StatusProvider * sprovider = STATUS_PROVIDER(g_object_new(provider_type, NULL));
	if (sprovider == NULL) {
		g_warning("Unable to build provider from: %s", provider);
		goto exit_module_fail;
	}

	/* On update let's talk to all of them and create the aggregate
	   value to export */
	g_signal_connect(G_OBJECT(sprovider), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED, G_CALLBACK(update_status), NULL);

	/* Attach the module object to the status provider so
	   that when the status provider is free'd the module
	   is close automatically. */
	g_object_set_data_full(G_OBJECT(sprovider), "status-provider-module", module, module_destroy_in_idle);

	status_providers = g_list_prepend(status_providers, sprovider);

	/* Force and update every time just so we know we're
	   in a consistent state*/
	update_status();

	goto exit_final;

exit_module_fail:
	g_module_close(module);

exit_final:
	g_free(provider);
	return FALSE;
}
