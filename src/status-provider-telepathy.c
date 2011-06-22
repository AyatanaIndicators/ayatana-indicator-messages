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

#include "status-provider.h"
#include "status-provider-telepathy.h"
#include "status-provider-telepathy-marshal.h"

#include <dbus/dbus-glib.h>

typedef enum {
	MC_STATUS_UNSET,
	MC_STATUS_OFFLINE,
	MC_STATUS_AVAILABLE,
	MC_STATUS_AWAY,
	MC_STATUS_EXTENDED_AWAY,
	MC_STATUS_HIDDEN,
	MC_STATUS_DND
} mc_status_t;

static StatusProviderStatus mc_to_sp_map[] = {
	/* MC_STATUS_UNSET,         */  STATUS_PROVIDER_STATUS_OFFLINE,
	/* MC_STATUS_OFFLINE,       */  STATUS_PROVIDER_STATUS_OFFLINE,
	/* MC_STATUS_AVAILABLE,     */  STATUS_PROVIDER_STATUS_ONLINE,
	/* MC_STATUS_AWAY,          */  STATUS_PROVIDER_STATUS_AWAY,
	/* MC_STATUS_EXTENDED_AWAY, */  STATUS_PROVIDER_STATUS_AWAY,
	/* MC_STATUS_HIDDEN,        */  STATUS_PROVIDER_STATUS_INVISIBLE,
	/* MC_STATUS_DND            */  STATUS_PROVIDER_STATUS_DND
};

static mc_status_t sp_to_mc_map[] = {
	/* STATUS_PROVIDER_STATUS_ONLINE,  */  MC_STATUS_AVAILABLE,
	/* STATUS_PROVIDER_STATUS_AWAY,    */  MC_STATUS_AWAY,
	/* STATUS_PROVIDER_STATUS_DND      */  MC_STATUS_DND,
	/* STATUS_PROVIDER_STATUS_INVISIBLE*/  MC_STATUS_HIDDEN,
	/* STATUS_PROVIDER_STATUS_OFFLINE  */  MC_STATUS_OFFLINE,
	/* STATUS_PROVIDER_STATUS_DISCONNECTED*/MC_STATUS_OFFLINE
};

typedef struct _StatusProviderTelepathyPrivate StatusProviderTelepathyPrivate;
struct _StatusProviderTelepathyPrivate {
	DBusGProxy * proxy;
	DBusGProxy * dbus_proxy;
	mc_status_t  mc_status;
};

#define STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), STATUS_PROVIDER_TELEPATHY_TYPE, StatusProviderTelepathyPrivate))

/* Prototypes */
/* GObject stuff */
static void status_provider_telepathy_class_init (StatusProviderTelepathyClass *klass);
static void status_provider_telepathy_init       (StatusProviderTelepathy *self);
static void status_provider_telepathy_dispose    (GObject *object);
static void status_provider_telepathy_finalize   (GObject *object);
/* Internal Funcs */
static void build_telepathy_proxy (StatusProviderTelepathy * self);
static void dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderTelepathy * self);
static void set_status (StatusProvider * sp, StatusProviderStatus status);
static StatusProviderStatus get_status (StatusProvider * sp);
static void changed_status (DBusGProxy * proxy, guint status, gchar * message, StatusProvider * sp);
static void proxy_destroy (DBusGProxy * proxy, StatusProvider * sp);
static void get_status_async (DBusGProxy * proxy, DBusGProxyCall * call, gpointer userdata);

G_DEFINE_TYPE (StatusProviderTelepathy, status_provider_telepathy, STATUS_PROVIDER_TYPE);

static void
status_provider_telepathy_class_init (StatusProviderTelepathyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (StatusProviderTelepathyPrivate));

	object_class->dispose = status_provider_telepathy_dispose;
	object_class->finalize = status_provider_telepathy_finalize;

	StatusProviderClass * spclass = STATUS_PROVIDER_CLASS(klass);

	spclass->set_status = set_status;
	spclass->get_status = get_status;

	return;
}


static void
status_provider_telepathy_init (StatusProviderTelepathy *self)
{
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(self);

	priv->proxy = NULL;
	priv->dbus_proxy = NULL;
	priv->mc_status = MC_STATUS_OFFLINE;

	GError * error = NULL;

	/* Grabbing the session bus */
	DBusGConnection * bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (bus == NULL) {
		g_warning("Unable to connect to Session Bus: %s", error == NULL ? "No message" : error->message);
		g_error_free(error);
		return;
	}

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

	build_telepathy_proxy(self);

	return;
}

/* Builds up the proxy to Mission Control and configures all of the
   signals for getting info from the proxy.  Also does a call to get
   the inital value of the status. */
static void
build_telepathy_proxy (StatusProviderTelepathy * self)
{
	g_debug("Building Telepathy Proxy");
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(self);

	if (priv->proxy != NULL) {
		g_debug("Hmm, being asked to build a proxy we alredy have.");
		return;
	}

	GError * error = NULL;

	/* Grabbing the session bus */
	DBusGConnection * session_bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (session_bus == NULL) {
		g_warning("Unable to connect to Session Bus: %s", error == NULL ? "No message" : error->message);
		g_error_free(error);
		return;
	}

	/* Get the proxy to Mission Control */
	priv->proxy = dbus_g_proxy_new_for_name_owner(session_bus,
	                         "org.freedesktop.Telepathy.MissionControl",
	                        "/org/freedesktop/Telepathy/MissionControl",
	                         "org.freedesktop.Telepathy.MissionControl",
	                         &error);

	if (priv->proxy != NULL) {
		/* If it goes, we set the proxy to NULL */
		g_object_add_weak_pointer (G_OBJECT(priv->proxy), (gpointer *)&priv->proxy);
		/* And we clean up other variables associated */
		g_signal_connect(G_OBJECT(priv->proxy), "destroy",
		                 G_CALLBACK(proxy_destroy), self);

		/* Set up the signal handler for watching when status changes. */
		dbus_g_object_register_marshaller(_status_provider_telepathy_marshal_VOID__UINT_STRING,
		                            G_TYPE_NONE,
		                            G_TYPE_UINT,
		                            G_TYPE_STRING,
		                            G_TYPE_INVALID);
		dbus_g_proxy_add_signal    (priv->proxy,
		                            "PresenceChanged",
		                            G_TYPE_UINT,
		                            G_TYPE_STRING,
		                            G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(priv->proxy,
		                            "PresenceChanged",
		                            G_CALLBACK(changed_status),
		                            (void *)self,
		                            NULL);

		/* Do a get here, to init the status */
		dbus_g_proxy_begin_call(priv->proxy,
		                        "GetStatus",
		                        get_status_async,
		                        self,
		                        NULL,
		                        G_TYPE_INVALID);
	} else {
		g_warning("Unable to connect to Mission Control");
		if (error != NULL) {
			g_error_free(error);
		}
	}

	return;
}

/* Watch to see if the Mission Control comes up on Dbus */
static void
dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderTelepathy * self)
{
	g_return_if_fail(name != NULL);
	g_return_if_fail(new != NULL);

	if (g_strcmp0(name, "org.freedesktop.Telepathy.MissionControl") == 0) {
		build_telepathy_proxy(self);
	}
	return;
}

static void
status_provider_telepathy_dispose (GObject *object)
{
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(object);

	if (priv->proxy != NULL) {
		g_object_unref(priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (status_provider_telepathy_parent_class)->dispose (object);
	return;
}

static void
status_provider_telepathy_finalize (GObject *object)
{

	G_OBJECT_CLASS (status_provider_telepathy_parent_class)->finalize (object);
	return;
}

/**
	status_provider_telepathy_new:

	Creates a new #StatusProviderTelepathy object.  No parameters or anything
	like that.  Just a convience function.

	Return value: A new instance of #StatusProviderTelepathy
*/
StatusProvider *
status_provider_telepathy_new (void)
{
	return STATUS_PROVIDER(g_object_new(STATUS_PROVIDER_TELEPATHY_TYPE, NULL));
}

static void
set_status (StatusProvider * sp, StatusProviderStatus status)
{
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(sp);
	if (priv->proxy == NULL) {
		priv->mc_status = MC_STATUS_OFFLINE;
		return;
	}

	priv->mc_status = sp_to_mc_map[status];	

	guint mcstatus = MC_STATUS_UNSET;
	gboolean ret = FALSE;
	GError * error = NULL;

	ret = dbus_g_proxy_call(priv->proxy,
	                        "GetPresence", &error,
	                        G_TYPE_INVALID,
	                        G_TYPE_UINT, &priv->mc_status,
	                        G_TYPE_INVALID);

	/* If we can't get the  get call to work, let's not set */
	if (!ret) {
		if (error != NULL) {
			g_error_free(error);
		}
		return;
	}
	
	/* If the get call doesn't return a status, that means that there
	   are no clients connected.  We don't want to connect them by telling
	   MC that we're going online -- we'd like to be more passive than that. */
	if (mcstatus == MC_STATUS_UNSET) {
		return;
	}

	ret = dbus_g_proxy_call(priv->proxy,
	                        "SetPresence", &error,
	                        G_TYPE_UINT, priv->mc_status,
	                        G_TYPE_STRING, "",
	                        G_TYPE_INVALID,
	                        G_TYPE_INVALID);

	if (!ret) {
		if (error != NULL) {
			g_warning("Unable to set Mission Control Presence: %s", error->message);
			g_error_free(error);
		} else {
			g_warning("Unable to set Mission Control Presence");
		}
		return;
	}

	return;
}

static StatusProviderStatus
get_status (StatusProvider * sp)
{
	g_return_val_if_fail(IS_STATUS_PROVIDER_TELEPATHY(sp), STATUS_PROVIDER_STATUS_DISCONNECTED);
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(sp);

	if (priv->proxy == NULL) {
		return STATUS_PROVIDER_STATUS_DISCONNECTED;
	}

	return mc_to_sp_map[priv->mc_status];
}

static void
changed_status (DBusGProxy * proxy, guint status, gchar * message, StatusProvider * sp)
{
	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(sp);
	priv->mc_status = status;
	g_signal_emit(G_OBJECT(sp), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, mc_to_sp_map[priv->mc_status], TRUE);
}

static void
proxy_destroy (DBusGProxy * proxy, StatusProvider * sp)
{
	g_debug("Signal: Mission Control proxy destroyed");
	g_signal_emit(G_OBJECT(sp), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, STATUS_PROVIDER_STATUS_OFFLINE, TRUE);
	return;
}

static void
get_status_async (DBusGProxy * proxy, DBusGProxyCall * call, gpointer userdata)
{
	GError * error = NULL;
	guint status = 0;
	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_UINT, &status, G_TYPE_INVALID)) {
		g_warning("Unable to get type from Mission Control: %s", error->message);
		g_error_free(error);
		return;
	}

	StatusProviderTelepathyPrivate * priv = STATUS_PROVIDER_TELEPATHY_GET_PRIVATE(userdata);

	gboolean changed = FALSE;
	if (status != priv->mc_status) {
		changed = TRUE;
	}

	priv->mc_status = status;

	if (changed) {
		g_signal_emit(G_OBJECT(userdata), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, mc_to_sp_map[priv->mc_status], TRUE);
	}

	return;
}
