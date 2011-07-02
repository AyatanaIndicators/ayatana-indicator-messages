
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
GList * menuitems = NULL;
GList * status_providers = NULL;

/* Build the inital status items and start kicking off the async code
   for handling all the statuses */
GList *
status_items_build (void)
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

	g_idle_add(provider_directory_parse, STATUS_PROVIDER_DIR);

	return menuitems;
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


	goto exit_final;

exit_module_fail:
	g_module_close(module);

exit_final:
	g_free(provider);
	return FALSE;
}
