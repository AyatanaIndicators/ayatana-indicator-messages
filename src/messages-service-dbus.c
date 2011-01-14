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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include "messages-service-dbus.h"
#include "dbus-data.h"
#include "gen-messages-service.xml.h"

enum {
	ATTENTION_CHANGED,
	ICON_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _MessageServiceDbusPrivate MessageServiceDbusPrivate;

struct _MessageServiceDbusPrivate
{
	gboolean dot;
	gboolean hidden;
};

#define MESSAGE_SERVICE_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbusPrivate))

static void message_service_dbus_class_init (MessageServiceDbusClass *klass);
static void message_service_dbus_init       (MessageServiceDbus *self);
static void message_service_dbus_dispose    (GObject *object);
static void message_service_dbus_finalize   (GObject *object);

static void _messages_service_server_watch  (void);
static gboolean _messages_service_server_attention_requested (MessageServiceDbus * self, gboolean * dot, GError ** error);
static gboolean _messages_service_server_icon_shown (MessageServiceDbus * self, gboolean * hidden, GError ** error);

static GDBusNodeInfo *            bus_node_info = NULL;
static GDBusInterfaceInfo *       bus_interface_info = NULL;

G_DEFINE_TYPE (MessageServiceDbus, message_service_dbus, G_TYPE_OBJECT);


static void
message_service_dbus_class_init (MessageServiceDbusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MessageServiceDbusPrivate));

	object_class->dispose = message_service_dbus_dispose;
	object_class->finalize = message_service_dbus_finalize;

	signals[ATTENTION_CHANGED] =  g_signal_new(MESSAGE_SERVICE_DBUS_SIGNAL_ATTENTION_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (MessageServiceDbusClass, attention_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__BOOLEAN,
	                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	signals[ICON_CHANGED] =  g_signal_new(MESSAGE_SERVICE_DBUS_SIGNAL_ICON_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (MessageServiceDbusClass, icon_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__BOOLEAN,
	                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	if (bus_node_info == NULL) {
		GError * error = NULL;

		bus_node_info = g_dbus_node_info_new_for_xml(_messages_service, &error);
		if (error != NULL) {
			g_error("Unable to parse Messaging Menu Interface description: %s", error->message);
			g_error_free(error);
		}
	}

	if (bus_interface_info == NULL) {
		bus_interface_info = g_dbus_node_info_lookup_interface(bus_node_info, INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE);

		if (bus_interface_info == NULL) {
			g_error("Unable to find interface '" INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE "'");
		}
	}

	return;
}

static void
connection_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GDBusConnection * connection = g_bus_get_finish(res, &error);

	if (error != NULL) {
		g_error("Unable to connect to the session bus: %s", error->message);
		g_error_free(error);
		return;
	}

	g_dbus_connection_register_object(connection,
	                                  INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
	                                  bus_interface_info,
	                                  bus_vtable,
	                                  user_data,
	                                  NULL, /* destroy */
	                                  &error);

	if (error != NULL) {
		g_error("Unable to register on session bus: %s", error->message);
		g_error_free(error);
		return;
	}

	g_debug("Service on session bus");

	return;
}

static void
message_service_dbus_init (MessageServiceDbus *self)
{
	g_bus_get(G_BUS_TYPE_SESSION, NULL, connection_cb, self);

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);

	priv->dot = FALSE;
	priv->hidden = FALSE;

	return;
}

static void
message_service_dbus_dispose (GObject *object)
{


	G_OBJECT_CLASS (message_service_dbus_parent_class)->dispose (object);
	return;
}

static void
message_service_dbus_finalize (GObject *object)
{


	G_OBJECT_CLASS (message_service_dbus_parent_class)->finalize (object);
	return;
}

MessageServiceDbus *
message_service_dbus_new (void)
{
	return MESSAGE_SERVICE_DBUS(g_object_new(MESSAGE_SERVICE_DBUS_TYPE, NULL));
}

/* DBus function to say that someone is watching */
static void
_messages_service_server_watch  (void)
{

}

/* DBus interface to request the private variable to know
   whether there is a green dot. */
static gboolean
_messages_service_server_attention_requested (MessageServiceDbus * self, gboolean * dot, GError ** error)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);
	*dot = priv->dot;
	return TRUE;
}

/* DBus interface to request the private variable to know
   whether the icon is hidden. */
static gboolean
_messages_service_server_icon_shown (MessageServiceDbus * self, gboolean * hidden, GError ** error)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);
	*hidden = priv->hidden;
	return TRUE;
}

void
message_service_dbus_set_attention (MessageServiceDbus * self, gboolean attention)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);
	/* Do signal */
	if (attention != priv->dot) {
		priv->dot = attention;
		g_signal_emit(G_OBJECT(self), signals[ATTENTION_CHANGED], 0, priv->dot, TRUE);
	}
	return;
}

void
message_service_dbus_set_icon (MessageServiceDbus * self, gboolean hidden)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);
	/* Do signal */
	if (hidden != priv->hidden) {
		priv->hidden = hidden;
		g_signal_emit(G_OBJECT(self), signals[ICON_CHANGED], 0, priv->hidden, TRUE);
	}
	return;
}
