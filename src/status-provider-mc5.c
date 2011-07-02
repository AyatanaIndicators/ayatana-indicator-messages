/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <telepathy-glib/account-manager.h>

#include "status-provider.h"
#include "status-provider-mc5.h"
#include "status-provider-mc5-marshal.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

static gchar * sp_to_mc_map[STATUS_PROVIDER_STATUS_LAST] = {
	/* STATUS_PROVIDER_STATUS_ONLINE,  */  "available",
	/* STATUS_PROVIDER_STATUS_AWAY,    */  "away",
	/* STATUS_PROVIDER_STATUS_DND      */  "busy",
	/* STATUS_PROVIDER_STATUS_INVISIBLE*/  "hidden",
	/* STATUS_PROVIDER_STATUS_OFFLINE  */  "offline",
	/* STATUS_PROVIDER_STATUS_DISCONNECTED*/NULL
};

static TpConnectionPresenceType sp_to_tp_map[STATUS_PROVIDER_STATUS_LAST] = {
	/* STATUS_PROVIDER_STATUS_ONLINE,  */    TP_CONNECTION_PRESENCE_TYPE_AVAILABLE,
	/* STATUS_PROVIDER_STATUS_AWAY,    */    TP_CONNECTION_PRESENCE_TYPE_AWAY,
	/* STATUS_PROVIDER_STATUS_DND      */    TP_CONNECTION_PRESENCE_TYPE_BUSY,
	/* STATUS_PROVIDER_STATUS_INVISIBLE*/    TP_CONNECTION_PRESENCE_TYPE_HIDDEN,
	/* STATUS_PROVIDER_STATUS_OFFLINE  */    TP_CONNECTION_PRESENCE_TYPE_OFFLINE,
	/* STATUS_PROVIDER_STATUS_DISCONNECTED*/ TP_CONNECTION_PRESENCE_TYPE_UNSET
};

static StatusProviderStatus tp_to_sp_map[TP_CONNECTION_PRESENCE_TYPE_ERROR + 1] = {
	/* TP_CONNECTION_PRESENCE_TYPE_UNSET          */  STATUS_PROVIDER_STATUS_DISCONNECTED,
	/* TP_CONNECTION_PRESENCE_TYPE_OFFLINE        */  STATUS_PROVIDER_STATUS_OFFLINE,
	/* TP_CONNECTION_PRESENCE_TYPE_AVAILABLE      */  STATUS_PROVIDER_STATUS_ONLINE,
	/* TP_CONNECTION_PRESENCE_TYPE_AWAY           */  STATUS_PROVIDER_STATUS_AWAY,
	/* TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY  */  STATUS_PROVIDER_STATUS_AWAY,
	/* TP_CONNECTION_PRESENCE_TYPE_HIDDEN         */  STATUS_PROVIDER_STATUS_INVISIBLE,
	/* TP_CONNECTION_PRESENCE_TYPE_BUSY           */  STATUS_PROVIDER_STATUS_DND,
	/* TP_CONNECTION_PRESENCE_TYPE_UNKNOWN        */  STATUS_PROVIDER_STATUS_DISCONNECTED,
	/* TP_CONNECTION_PRESENCE_TYPE_ERROR          */  STATUS_PROVIDER_STATUS_DISCONNECTED
};

typedef struct _StatusProviderMC5Private StatusProviderMC5Private;
struct _StatusProviderMC5Private {
	TpAccountManager * manager;
	StatusProviderStatus status;
	DBusGProxy * dbus_proxy;
};

#define STATUS_PROVIDER_MC5_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), STATUS_PROVIDER_MC5_TYPE, StatusProviderMC5Private))
#define MC5_WELL_KNOWN_NAME  "org.freedesktop.Telepathy.AccountManager"

/* Prototypes */
/* GObject stuff */
static void status_provider_mc5_class_init (StatusProviderMC5Class *klass);
static void status_provider_mc5_init       (StatusProviderMC5 *self);
static void status_provider_mc5_dispose    (GObject *object);
static void status_provider_mc5_finalize   (GObject *object);
/* Internal Funcs */
static void set_status (StatusProvider * sp, StatusProviderStatus status);
static StatusProviderStatus get_status (StatusProvider * sp);
static void presence_changed (TpAccountManager * eam, guint type, const gchar * type_str, const gchar * message, StatusProviderMC5 * sp);
static void dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderMC5 * self);
static void mc5_exists_cb (DBusGProxy * proxy, gboolean exists, GError * error, gpointer userdata);

G_DEFINE_TYPE (StatusProviderMC5, status_provider_mc5, STATUS_PROVIDER_TYPE);

STATUS_PROVIDER_EXPORT_TYPE(STATUS_PROVIDER_MC5_TYPE)

/* Create the class.  We over ride a few functions but nothing
   really shocking.  Most interesting is the set and get status. */
static void
status_provider_mc5_class_init (StatusProviderMC5Class *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (StatusProviderMC5Private));

	object_class->dispose = status_provider_mc5_dispose;
	object_class->finalize = status_provider_mc5_finalize;

	StatusProviderClass * spclass = STATUS_PROVIDER_CLASS(klass);

	spclass->set_status = set_status;
	spclass->get_status = get_status;

	return;
}

/* Build our telepathy account manager instance if we don't
   have one. */
static void
build_eam (StatusProviderMC5 * self)
{
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(self);

	if (priv->manager != NULL) {
		return;
	}

	priv->manager = tp_account_manager_dup();
	g_signal_connect(G_OBJECT(priv->manager), "most-available-presence-changed", G_CALLBACK(presence_changed), self);

	return;
}

/* Creating an instance of the status provider.  We set the variables
   and create an TpAccountManager object.  It does all the hard
   work in this module of tracking MissionControl and enumerating the
   accounts and all that jazz. */
static void
status_provider_mc5_init (StatusProviderMC5 *self)
{
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(self);

	priv->status = STATUS_PROVIDER_STATUS_DISCONNECTED;
	priv->manager = NULL;

	DBusGConnection * bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	g_return_if_fail(bus != NULL); /* Can't do anymore DBus stuff without this,
	                                  all non-DBus stuff should be done */

	GError * error = NULL;

	/* Set up the dbus Proxy */
	priv->dbus_proxy = dbus_g_proxy_new_for_name_owner (bus,
	                                                    DBUS_SERVICE_DBUS,
	                                                    DBUS_PATH_DBUS,
	                                                    DBUS_INTERFACE_DBUS,
	                                                    &error);
	if (error != NULL) {
		g_warning("Unable to connect to DBus events: %s", error->message);
		g_error_free(error);
		return;
	}

	/* Configure the name owner changing */
	dbus_g_proxy_add_signal(priv->dbus_proxy, "NameOwnerChanged",
	                        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
							G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->dbus_proxy, "NameOwnerChanged",
	                        G_CALLBACK(dbus_namechange),
	                        self, NULL);

	org_freedesktop_DBus_name_has_owner_async(priv->dbus_proxy, MC5_WELL_KNOWN_NAME, mc5_exists_cb, self);

	return;
}

/* Unref the account manager and move on.  Sadly, we're
   leaving the show. */
static void
status_provider_mc5_dispose (GObject *object)
{
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(object);

	if (priv->manager != NULL) {
		g_object_unref(priv->manager);
		priv->manager = NULL;
	}

	if (priv->dbus_proxy != NULL) {
		g_object_unref(priv->dbus_proxy);
		priv->dbus_proxy = NULL;
	}

	G_OBJECT_CLASS (status_provider_mc5_parent_class)->dispose (object);
	return;
}

/* Pass to superclass */
static void
status_provider_mc5_finalize (GObject *object)
{

	G_OBJECT_CLASS (status_provider_mc5_parent_class)->finalize (object);
	return;
}

/* Watch for MC5 Coming on and off the bus. */
static void
dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderMC5 * self)
{
	/* g_debug("DBUS NAMECHANGE: %s %s %s", name, prev, new); */

	if (prev[0] == '\0' && g_strcmp0(name, MC5_WELL_KNOWN_NAME) == 0) {
		g_debug("MC5 Coming online");
		build_eam(self);
	}
	if (new[0] == '\0' && g_strcmp0(name, MC5_WELL_KNOWN_NAME) == 0) {
		g_debug("MC5 going offline");
		StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(self);
		if (priv->manager != NULL) {
			g_object_unref(priv->manager);
			priv->manager = NULL;
		}

		priv->status = STATUS_PROVIDER_STATUS_DISCONNECTED;
		g_signal_emit(G_OBJECT(self), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, priv->status, TRUE);
	}

	return;
}

/* Callback for the Dbus command to do HasOwner on
   the MC5 service.  If it exists, we want to have an
   account manager. */
static void
mc5_exists_cb (DBusGProxy * proxy, gboolean exists, GError * error, gpointer userdata)
{
	if (error) {
		g_warning("Unable to check if MC5 is running: %s", error->message);
		return;
	}

	if (exists) {
		build_eam(STATUS_PROVIDER_MC5(userdata));
	}

	return;
}

/**
	status_provider_mc5_new:

	Creates a new #StatusProviderMC5 object.  No parameters or anything
	like that.  Just a convience function.

	Return value: A new instance of #StatusProviderMC5
*/
StatusProvider *
status_provider_mc5_new (void)
{
	return STATUS_PROVIDER(g_object_new(STATUS_PROVIDER_MC5_TYPE, NULL));
}

/* Setting the status in the empathy account manager.  We're
   basically requesting a global status.  This may or may not
   get applied to all accounts.  It's really the best we can
   hope to do. */
static void
set_status (StatusProvider * sp, StatusProviderStatus status)
{
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(sp);

	build_eam(STATUS_PROVIDER_MC5(sp));
	tp_account_manager_set_all_requested_presences(priv->manager, sp_to_tp_map[status], sp_to_mc_map[status], "");

	return;
}

/* Gets the status, uses the cached value that we have.  Asking
   would just be painful. */
static StatusProviderStatus
get_status (StatusProvider * sp)
{
	g_return_val_if_fail(IS_STATUS_PROVIDER_MC5(sp), STATUS_PROVIDER_STATUS_DISCONNECTED);
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(sp);

	if (priv->manager == NULL) {
		return STATUS_PROVIDER_STATUS_DISCONNECTED;
	}

	return priv->status;
}

/* A signal handler for when the TpAccountManager believes
   that the global status has changed.  It roughly calculates this
   by finding the most available of all accounts that are active. */
static void
presence_changed (TpAccountManager * eam, guint type, const gchar * type_str, const gchar * message, StatusProviderMC5 * sp)
{
	StatusProviderMC5Private * priv = STATUS_PROVIDER_MC5_GET_PRIVATE(sp);

	g_debug("MC5 Status changed: %d %s %s", type, type_str, message);

	if (priv->status != tp_to_sp_map[type]) {
		priv->status = tp_to_sp_map[type];
		g_signal_emit(G_OBJECT(sp), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, priv->status, TRUE);
	}

	return;
}

