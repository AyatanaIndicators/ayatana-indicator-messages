
#include <glib.h>
#include <glib/gi18n.h>
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

static StatusProviderStatus current_status = STATUS_PROVIDER_STATUS_DISCONNECTED;
GList * menuitems = NULL;

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
