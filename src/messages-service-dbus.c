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
	GDBusConnection * connection;
	GCancellable    * accounts_cancel;
	GDBusProxy      * accounts_user;
	gboolean dot;
	gboolean hidden;
};

#define MESSAGE_SERVICE_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbusPrivate))

static void message_service_dbus_class_init (MessageServiceDbusClass *klass);
static void message_service_dbus_init       (MessageServiceDbus *self);
static void message_service_dbus_dispose    (GObject *object);
static void message_service_dbus_finalize   (GObject *object);
static void     bus_method_call             (GDBusConnection * connection,
                                             const gchar * sender,
                                             const gchar * path,
                                             const gchar * interface,
                                             const gchar * method,
                                             GVariant * params,
                                             GDBusMethodInvocation * invocation,
                                             gpointer user_data);

static GDBusNodeInfo *            bus_node_info = NULL;
static GDBusInterfaceInfo *       bus_interface_info = NULL;
static const GDBusInterfaceVTable bus_interface_table = {
	method_call:    bus_method_call,
	get_property:   NULL,  /* No properties */
	set_property:   NULL   /* No properties */
};

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

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(user_data);
	priv->connection = connection;

	g_dbus_connection_register_object(connection,
	                                  INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
	                                  bus_interface_info,
	                                  &bus_interface_table,
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
accounts_notify_cb (GObject *source_object, GAsyncResult *res,
                    gpointer user_data)
{
	GError * error = NULL;
	GVariant * answer = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);

	if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free(error);
		return; /* Must exit before accessing freed memory */
	}

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(user_data);

	if (priv->accounts_cancel != NULL) {
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	if (error != NULL) {
		g_warning("Unable to get notify accounts service of message status: %s", error->message);
		g_error_free(error);
		return;
	}

	g_variant_unref (answer);
}

static void
accounts_notify (MessageServiceDbus *self)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);

	if (priv->accounts_user == NULL)
		return; /* We're not able to talk to accounts service */

	if (priv->accounts_cancel != NULL) {
		/* Cancel old notify before starting new one */
		g_cancellable_cancel(priv->accounts_cancel);
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	priv->accounts_cancel = g_cancellable_new();
	g_dbus_proxy_call(priv->accounts_user,
	                  "SetXHasMessages",
	                  g_variant_new ("(b)", priv->dot),
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1, /* timeout */
	                  priv->accounts_cancel,
	                  accounts_notify_cb,
	                  self);
}

static void
get_accounts_user_proxy_cb (GObject *source_object, GAsyncResult *res,
                            gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free(error);
		return; /* Must exit before accessing freed memory */
	}

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(user_data);

	if (priv->accounts_cancel != NULL) {
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	if (error != NULL) {
		g_warning("Unable to get proxy of accountsservice: %s", error->message);
		g_error_free(error);
		return;
	}

	priv->accounts_user = proxy;
	accounts_notify (MESSAGE_SERVICE_DBUS (user_data));
}

static void
get_accounts_user_find_user_cb (GObject *source_object, GAsyncResult *res,
                                gpointer user_data)
{
	GError * error = NULL;
	GVariant * answer = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);

	/* We're done with main accounts proxy now */
	g_object_unref (source_object);

	if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free(error);
		return; /* Must exit before accessing freed memory */
	}

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(user_data);

	if (priv->accounts_cancel != NULL) {
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	if (error != NULL) {
		g_warning("Unable to get object name of user from accountsservice: %s", error->message);
		g_error_free(error);
		return;
	}

	if (!g_variant_is_of_type (answer, G_VARIANT_TYPE ("(o)"))) {
		g_warning("Unexpected type from FindUserByName: %s", g_variant_get_type_string (answer));
		g_variant_unref(answer);
		return;
	}

	const gchar *path;
	g_variant_get(answer, "(&o)", &path);

	priv->accounts_cancel = g_cancellable_new();
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
	                         G_DBUS_PROXY_FLAGS_NONE,
	                         NULL,
	                         "org.freedesktop.Accounts", 
	                         path,
	                         "org.freedesktop.Accounts.User",
	                         priv->accounts_cancel,
	                         get_accounts_user_proxy_cb,
	                         user_data);

	g_variant_unref (answer);
}

static void
get_accounts_proxy_cb (GObject *source_object, GAsyncResult *res,
                       gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free(error);
		return; /* Must exit before accessing freed memory */
	}

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(user_data);

	if (priv->accounts_cancel != NULL) {
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	if (error != NULL) {
		g_warning("Unable to get proxy of accountsservice: %s", error->message);
		g_error_free(error);
		return;
	}

	priv->accounts_cancel = g_cancellable_new();
	g_dbus_proxy_call(proxy,
	                  "FindUserByName",
	                  g_variant_new ("(s)", g_get_user_name ()),
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1, /* timeout */
	                  priv->accounts_cancel,
	                  get_accounts_user_find_user_cb,
	                  user_data);
}

static void
get_accounts_proxy (MessageServiceDbus *self)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);

	g_return_if_fail(priv->accounts_cancel == NULL);

	priv->accounts_cancel = g_cancellable_new();
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
	                         G_DBUS_PROXY_FLAGS_NONE,
	                         NULL,
	                         "org.freedesktop.Accounts", 
	                         "/org/freedesktop/Accounts",
	                         "org.freedesktop.Accounts",
	                         priv->accounts_cancel,
	                         get_accounts_proxy_cb,
	                         self);
}

static void
message_service_dbus_init (MessageServiceDbus *self)
{
	g_bus_get(G_BUS_TYPE_SESSION, NULL, connection_cb, self);
	get_accounts_proxy (self);

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);

	priv->dot = FALSE;
	priv->hidden = FALSE;

	return;
}

static void
message_service_dbus_dispose (GObject *object)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(object);

	if (priv->connection != NULL) {
		g_object_unref(priv->connection);
		priv->connection = NULL;
	}

	if (priv->accounts_cancel != NULL) {
		g_cancellable_cancel(priv->accounts_cancel);
		g_object_unref(priv->accounts_cancel);
		priv->accounts_cancel = NULL;
	}

	if (priv->accounts_user != NULL) {
		g_object_unref(priv->accounts_user);
		priv->accounts_user = NULL;
	}

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

/* Method request off of DBus */
static void
bus_method_call (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * method, GVariant * params, GDBusMethodInvocation * invocation, gpointer user_data)
{
	MessageServiceDbus * ms = MESSAGE_SERVICE_DBUS(user_data);
	if (ms == NULL) { return; }

	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(ms);

	if (g_strcmp0("AttentionRequested", method) == 0) {
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", priv->dot));
		return;
	} else if (g_strcmp0("IconShown", method) == 0) {
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", priv->hidden));
		return;
	} else if (g_strcmp0("ClearAttention", method) == 0) {
		message_service_dbus_set_attention(ms, FALSE);
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	} else {
		g_warning("Unknown function call '%s'", method);
	}

	return;
}

void
message_service_dbus_set_attention (MessageServiceDbus * self, gboolean attention)
{
	MessageServiceDbusPrivate * priv = MESSAGE_SERVICE_DBUS_GET_PRIVATE(self);
	/* Do signal */
	if (attention != priv->dot) {
		priv->dot = attention;
		g_signal_emit(G_OBJECT(self), signals[ATTENTION_CHANGED], 0, priv->dot, TRUE);

		if (priv->connection != NULL) {
			g_dbus_connection_emit_signal(priv->connection,
			                              NULL,
			                              INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
			                              INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE,
			                              "AttentionChanged",
			                              g_variant_new("(b)", priv->dot),
			                              NULL);
		}

		accounts_notify (self);
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

		if (priv->connection != NULL) {
			g_dbus_connection_emit_signal(priv->connection,
			                              NULL,
			                              INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
			                              INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE,
			                              "IconChanged",
			                              g_variant_new("(b)", priv->hidden),
			                              NULL);
		}
	}
	return;
}
