/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2011 Canonical Ltd.

Authors:
    Stefano Candori <stefano.candori@gmail.com>

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
#include "status-provider-emesene.h"

#include <dbus/dbus-glib.h>

typedef enum {
        EM_STATUS_ONLINE,
	EM_STATUS_OFFLINE,
	EM_STATUS_BUSY,
	EM_STATUS_AWAY,
	EM_STATUS_IDLE
} em_status_t;

static const StatusProviderStatus em_to_sp_map[] = {
	/* EM_STATUS_ONLINE,         */   STATUS_PROVIDER_STATUS_ONLINE,
	/* EM_STATUS_OFFLINE,        */   STATUS_PROVIDER_STATUS_OFFLINE,
	/* EM_STATUS_BUSY,           */   STATUS_PROVIDER_STATUS_DND,
	/* EM_STATUS_AWAY,           */   STATUS_PROVIDER_STATUS_AWAY,
	/* EM_STATUS_IDLE,           */   STATUS_PROVIDER_STATUS_AWAY
};

static const em_status_t sp_to_em_map[STATUS_PROVIDER_STATUS_LAST] = {
	/* STATUS_PROVIDER_STATUS_ONLINE,  */  EM_STATUS_ONLINE,
	/* STATUS_PROVIDER_STATUS_AWAY,    */  EM_STATUS_AWAY,
	/* STATUS_PROVIDER_STATUS_DND      */  EM_STATUS_BUSY,
	/* STATUS_PROVIDER_STATUS_INVISIBLE*/  EM_STATUS_OFFLINE,
	/* STATUS_PROVIDER_STATUS_OFFLINE  */  EM_STATUS_OFFLINE,
	/* STATUS_PROVIDER_STATUS_DISCONNECTED*/ EM_STATUS_OFFLINE
};

typedef struct _StatusProviderEmesenePrivate StatusProviderEmesenePrivate;
struct _StatusProviderEmesenePrivate {
	DBusGProxy * proxy;
	DBusGProxy * dbus_proxy;
	em_status_t  em_status;
};

#define STATUS_PROVIDER_EMESENE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), STATUS_PROVIDER_EMESENE_TYPE, StatusProviderEmesenePrivate))

/* Prototypes */
/* GObject stuff */
static void status_provider_emesene_class_init (StatusProviderEmeseneClass *klass);
static void status_provider_emesene_init       (StatusProviderEmesene *self);
static void status_provider_emesene_dispose    (GObject *object);
static void status_provider_emesene_finalize   (GObject *object);
/* Internal Funcs */
static void set_status (StatusProvider * sp, StatusProviderStatus status);
static StatusProviderStatus get_status (StatusProvider * sp);
static void setup_emesene_proxy (StatusProviderEmesene * self);
static void dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderEmesene * self);

G_DEFINE_TYPE (StatusProviderEmesene, status_provider_emesene, STATUS_PROVIDER_TYPE);

STATUS_PROVIDER_EXPORT_TYPE(STATUS_PROVIDER_EMESENE_TYPE)

static void
status_provider_emesene_class_init (StatusProviderEmeseneClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (StatusProviderEmesenePrivate));

	object_class->dispose = status_provider_emesene_dispose;
	object_class->finalize = status_provider_emesene_finalize;

	StatusProviderClass * spclass = STATUS_PROVIDER_CLASS(klass);

	spclass->set_status = set_status;
	spclass->get_status = get_status;

	return;
}

static void
status_cb (DBusGProxy * proxy, DBusGProxyCall * call, gpointer userdata)
{
        StatusProviderEmesene * spe = STATUS_PROVIDER_EMESENE(userdata);
        StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(spe);
        
	GError * error = NULL;
	gint status = 0;
	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INT, &status, G_TYPE_INVALID)) {
		g_warning("Unable to get status from Emesene: %s", error->message);
		g_error_free(error);
		return;
	}

	priv->em_status = status;
	g_signal_emit(G_OBJECT(spe), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, em_to_sp_map[priv->em_status], TRUE);
	return;
}

static void
changed_status (DBusGProxy * proxy, gint status, StatusProviderEmesene * spe)
{
        StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(spe);
        g_debug("Emesene changed status to %d", status);
        priv->em_status = status;
	g_signal_emit(G_OBJECT(spe), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, em_to_sp_map[priv->em_status], TRUE);
	return;
}

static void
proxy_destroy (DBusGProxy * proxy, StatusProviderEmesene * spe)
{
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(spe);

	priv->proxy = NULL;
	priv->em_status = EM_STATUS_OFFLINE;

	g_signal_emit(G_OBJECT(spe), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, em_to_sp_map[priv->em_status], TRUE);
	return;
}

static void
status_provider_emesene_init (StatusProviderEmesene *self)
{
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(self);

	priv->proxy = NULL;
	priv->em_status = EM_STATUS_OFFLINE;

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

	setup_emesene_proxy(self);

	return;
}

/* Watch to see if the Emesene comes up on Dbus */
static void
dbus_namechange (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, StatusProviderEmesene * self)
{
	g_return_if_fail(name != NULL);
	g_return_if_fail(new != NULL);

	if (g_strcmp0(name, "org.emesene.Service") == 0) {
		setup_emesene_proxy(self);
	}
	return;
}

/* Setup the Emesene proxy so that we can talk to it
   and get signals from it.  */
static void
setup_emesene_proxy (StatusProviderEmesene * self)
{
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(self);

	if (priv->proxy != NULL) {
		g_debug("Doh!We were asked to set up a Emesene proxy when we already had one.");
		return;
	}

	DBusGConnection * bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	g_return_if_fail(bus != NULL); /* Can't do anymore DBus stuff without this,
	                                  all non-DBus stuff should be done */

	GError * error = NULL;

	/* Set up the Emesene Proxy */
	priv->proxy = dbus_g_proxy_new_for_name_owner (bus,
	                                               "org.emesene.Service",
	                                               "/org/emesene/Service",
	                                               "org.emesene.Service",
	                                               &error);
	/* Report any errors */
	if (error != NULL) {
		g_debug("Unable to get Emesene proxy: %s", error->message);
		g_error_free(error);
	}

	/* If we have a proxy, let's start using it */
	if (priv->proxy != NULL) {
		/* Set the proxy to NULL if it's destroyed */
		g_object_add_weak_pointer (G_OBJECT(priv->proxy), (gpointer *)&priv->proxy);
		/* If it's destroyed, let's clean up as well */
		g_signal_connect(G_OBJECT(priv->proxy), "destroy",
		                 G_CALLBACK(proxy_destroy), self);

		/* Watching for the status change coming from the
		   Emesene side of things. */
		g_debug("Adding Emesene Signals");
		dbus_g_proxy_add_signal    (priv->proxy,
		                            "status_changed",
		                            G_TYPE_INT,
		                            G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(priv->proxy,
		                            "status_changed",
		                            G_CALLBACK(changed_status),
		                            (void *)self,
		                            NULL);

		/* Get the current status to update our cached
		   value of the status. */
		dbus_g_proxy_begin_call(priv->proxy,
		                        "get_status",
		                        status_cb,
		                        self,
		                        NULL,
		                        G_TYPE_INVALID);
	}

	return;
}

static void
status_provider_emesene_dispose (GObject *object)
{
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(object);

	if (priv->proxy != NULL) {
		g_object_unref(priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (status_provider_emesene_parent_class)->dispose (object);
	return;
}

static void
status_provider_emesene_finalize (GObject *object)
{

	G_OBJECT_CLASS (status_provider_emesene_parent_class)->finalize (object);
	return;
}

/**
	status_provider_emesene_new:

	Creates a new #StatusProviderEmesene object.  No parameters or anything
	like that.  Just a convience function.

	Return value: A new instance of #StatusProviderEmesene
*/
StatusProvider *
status_provider_emesene_new (void)
{
	return STATUS_PROVIDER(g_object_new(STATUS_PROVIDER_EMESENE_TYPE, NULL));
}

/* Takes the status provided generically for Status providers
   and turns it into a Emesene status and sends it to Emesene. */
static void
set_status (StatusProvider * sp, StatusProviderStatus status)
{
	g_return_if_fail(IS_STATUS_PROVIDER_EMESENE(sp));
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(sp);

	g_debug("Emesene set status to %d", status);
	if (priv->proxy == NULL) {
		return;
	}

	priv->em_status = sp_to_em_map[status];
	
	gboolean ret = FALSE;
	GError * error = NULL;

	ret = dbus_g_proxy_call(priv->proxy,
	                        "set_status", &error,
				G_TYPE_INT, priv->em_status,
	                        G_TYPE_INVALID,
	                        G_TYPE_INVALID);

	if (!ret) {
		if (error != NULL) {
			g_warning("Emesene unable to change to status: %s", error->message);
			g_error_free(error);
		} else {
			g_warning("Emesene unable to change to status");
		}
		error = NULL;
	}

	g_signal_emit(G_OBJECT(sp), STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID, 0, em_to_sp_map[priv->em_status], TRUE);
	return;
}

/* Takes the cached Emesene status and makes it into the generic
   Status provider status.  If there is no Emesene proxy then it
   returns the disconnected state. */
static StatusProviderStatus
get_status (StatusProvider * sp)
{
	g_return_val_if_fail(IS_STATUS_PROVIDER_EMESENE(sp), STATUS_PROVIDER_STATUS_DISCONNECTED);
	StatusProviderEmesenePrivate * priv = STATUS_PROVIDER_EMESENE_GET_PRIVATE(sp);

	if (priv->proxy == NULL) {
		return STATUS_PROVIDER_STATUS_DISCONNECTED;
	}

	return em_to_sp_map[priv->em_status];
}
