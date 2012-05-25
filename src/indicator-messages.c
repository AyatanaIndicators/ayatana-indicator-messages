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

#include "config.h"

#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-image-helper.h>
#include <libindicator/indicator-service-manager.h>

#include <menu-factory-gtk.h>

#include "dbus-data.h"
#include "gen-messages-service.xml.h"

#define INDICATOR_MESSAGES_TYPE            (indicator_messages_get_type ())
#define INDICATOR_MESSAGES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_MESSAGES_TYPE, IndicatorMessages))
#define INDICATOR_MESSAGES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_MESSAGES_TYPE, IndicatorMessagesClass))
#define IS_INDICATOR_MESSAGES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_MESSAGES_TYPE))
#define IS_INDICATOR_MESSAGES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_MESSAGES_TYPE))
#define INDICATOR_MESSAGES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_MESSAGES_TYPE, IndicatorMessagesClass))

typedef struct _IndicatorMessages      IndicatorMessages;
typedef struct _IndicatorMessagesClass IndicatorMessagesClass;

struct _IndicatorMessagesClass {
	IndicatorObjectClass parent_class;
	void    (*update_a11y_desc) (IndicatorServiceManager * service, gpointer * user_data);
};

struct _IndicatorMessages {
	IndicatorObject parent;
	IndicatorServiceManager * service;
	GActionGroup *actions;
	GMenuModel *menu;
};

GType indicator_messages_get_type (void);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_MESSAGES_TYPE)

/* Globals */
static GtkWidget * main_image = NULL;
static GDBusProxy * icon_proxy = NULL;
static GDBusNodeInfo *            bus_node_info = NULL;
static GDBusInterfaceInfo *       bus_interface_info = NULL;
static const gchar *              accessible_desc = NULL;
static IndicatorObject *          indicator = NULL;

/* Prototypes */
static void indicator_messages_class_init (IndicatorMessagesClass *klass);
static void indicator_messages_init       (IndicatorMessages *self);
static void indicator_messages_dispose    (GObject *object);
static void indicator_messages_finalize   (GObject *object);
static GtkImage * get_icon                (IndicatorObject * io);
static GtkMenu * get_menu                 (IndicatorObject * io);
static void indicator_messages_middle_click (IndicatorObject * io,
                                             IndicatorObjectEntry * entry,
                                             guint time, gpointer data);
static const gchar * get_accessible_desc  (IndicatorObject * io);
static const gchar * get_name_hint        (IndicatorObject * io);
static void connection_change             (IndicatorServiceManager * sm,
                                           gboolean connected,
                                           gpointer user_data);

G_DEFINE_TYPE (IndicatorMessages, indicator_messages, INDICATOR_OBJECT_TYPE);

static void
update_a11y_desc (void)
{
	g_return_if_fail(IS_INDICATOR_MESSAGES(indicator));

	GList *entries = indicator_object_get_entries(indicator);
	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;

	entry->accessible_desc = get_accessible_desc(indicator);

	g_signal_emit(G_OBJECT(indicator),
	              INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID,
	              0,
	              entry,
	              TRUE);

	g_list_free(entries);

	return;
}

/* Initialize the one-timers */
static void
indicator_messages_class_init (IndicatorMessagesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_messages_dispose;
	object_class->finalize = indicator_messages_finalize;

	IndicatorObjectClass * io_class = INDICATOR_OBJECT_CLASS(klass);

	io_class->get_image = get_icon;
	io_class->get_menu = get_menu;
	io_class->get_accessible_desc = get_accessible_desc;
	io_class->get_name_hint = get_name_hint;
	io_class->secondary_activate = indicator_messages_middle_click;

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

/* Build up our per-instance variables */
static void
indicator_messages_init (IndicatorMessages *self)
{
	GDBusConnection *bus;
	GError *error = NULL;

	/* Default values */
	self->service = NULL;

	/* Complex stuff */
	self->service = indicator_service_manager_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);
	g_signal_connect(self->service, INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE, G_CALLBACK(connection_change), self);

	bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (!bus) {
		g_warning ("error connecting to the session bus: %s", error->message);
		g_error_free (error);
		return;
	}

	self->actions = G_ACTION_GROUP (g_dbus_action_group_get (bus,
								 INDICATOR_MESSAGES_DBUS_NAME,
								 INDICATOR_MESSAGES_DBUS_OBJECT));

	self->menu = G_MENU_MODEL (g_dbus_menu_model_get (bus,
							  INDICATOR_MESSAGES_DBUS_NAME,
							  INDICATOR_MESSAGES_DBUS_OBJECT));

	indicator = INDICATOR_OBJECT(self);

	g_object_unref (bus);
}

/* Unref stuff */
static void
indicator_messages_dispose (GObject *object)
{
	IndicatorMessages * self = INDICATOR_MESSAGES(object);
	g_return_if_fail(self != NULL);

	if (self->service != NULL) {
		g_object_unref(self->service);
		self->service = NULL;
	}

	g_clear_object (&self->actions);
	g_clear_object (&self->menu);

	G_OBJECT_CLASS (indicator_messages_parent_class)->dispose (object);
	return;
}

/* Destory all memory users */
static void
indicator_messages_finalize (GObject *object)
{

	G_OBJECT_CLASS (indicator_messages_parent_class)->finalize (object);
	return;
}



/* Functions */

/* Signal off of the proxy */
static void
proxy_signal (GDBusProxy * proxy, const gchar * sender, const gchar * signal, GVariant * params, gpointer user_data)
{
	gboolean prop = g_variant_get_boolean(g_variant_get_child_value(params, 0));

	if (g_strcmp0("AttentionChanged", signal) == 0) {
		if (prop) {
			indicator_image_helper_update(GTK_IMAGE(main_image), "indicator-messages-new");
			accessible_desc = _("New Messages");
		} else {
			indicator_image_helper_update(GTK_IMAGE(main_image), "indicator-messages");
			accessible_desc = _("Messages");
		}
	} else if (g_strcmp0("IconChanged", signal) == 0) {
		if (prop) {
			gtk_widget_hide(main_image);
		} else {
			gtk_widget_show(main_image);
		}
	} else {
		g_warning("Unknown signal %s", signal);
	}

	update_a11y_desc();

	return;
}

/* Callback from getting the attention status from the service. */
static void
attention_cb (GObject * object, GAsyncResult * ares, gpointer user_data)
{
	GError * error = NULL;
	GVariant * res = g_dbus_proxy_call_finish(G_DBUS_PROXY(object), ares, &error);

	if (error != NULL) {
		g_warning("Unable to get attention status: %s", error->message);
		g_error_free(error);
		return;
	}

	gboolean prop = g_variant_get_boolean(g_variant_get_child_value(res, 0));

	if (prop) {
		indicator_image_helper_update(GTK_IMAGE(main_image), "indicator-messages-new");
		accessible_desc = _("New Messages");
	} else {
		indicator_image_helper_update(GTK_IMAGE(main_image), "indicator-messages");
		accessible_desc = _("Messages");
	}

	update_a11y_desc();

	return;
}

/* Change from getting the icon visibility from the service */
static void
icon_cb (GObject * object, GAsyncResult * ares, gpointer user_data)
{
	GError * error = NULL;
	GVariant * res = g_dbus_proxy_call_finish(G_DBUS_PROXY(object), ares, &error);

	if (error != NULL) {
		g_warning("Unable to get icon visibility: %s", error->message);
		g_error_free(error);
		return;
	}

	gboolean prop = g_variant_get_boolean(g_variant_get_child_value(res, 0));
	
	if (prop) {
		gtk_widget_hide(main_image);
	} else {
		gtk_widget_show(main_image);
	}

	return;
}

static guint connection_drop_timeout = 0;

/* Resets the icon to not having messages if we can't get a good
   answer on it from the service. */
static gboolean
connection_drop_cb (gpointer user_data)
{
	if (main_image != NULL) {
		indicator_image_helper_update(GTK_IMAGE(main_image), "indicator-messages");
	}
	connection_drop_timeout = 0;
	return FALSE;
}

/* Proxy is setup now.. whoo! */
static void
proxy_ready_cb (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (error != NULL) {
		g_warning("Unable to get proxy of service: %s", error->message);
		g_error_free(error);
		return;
	}

	icon_proxy = proxy;

	g_signal_connect(G_OBJECT(proxy), "g-signal", G_CALLBACK(proxy_signal), user_data);

	g_dbus_proxy_call(icon_proxy,
	                  "AttentionRequested",
	                  NULL, /* params */
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1, /* timeout */
	                  NULL, /* cancel */
	                  attention_cb,
	                  user_data);
	g_dbus_proxy_call(icon_proxy,
	                  "IconShown",
	                  NULL, /* params */
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1, /* timeout */
	                  NULL, /* cancel */
	                  icon_cb,
	                  user_data);

	return;
}

/* Sets up all the icon information in the proxy. */
static void 
connection_change (IndicatorServiceManager * sm, gboolean connected, gpointer user_data)
{
	if (connection_drop_timeout != 0) {
		g_source_remove(connection_drop_timeout);
		connection_drop_timeout = 0;
	}

	if (!connected) {
		/* Ensure that we're not saying there are messages
		   when we don't have a connection. */
		connection_drop_timeout = g_timeout_add(400, connection_drop_cb, NULL);
		return;
	}

	if (icon_proxy == NULL) {
		g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
		                         G_DBUS_PROXY_FLAGS_NONE,
		                         bus_interface_info,
		                         INDICATOR_MESSAGES_DBUS_NAME,
		                         INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
		                         INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE,
		                         NULL, /* cancel */
		                         proxy_ready_cb,
		                         sm);
	} else {
		g_dbus_proxy_call(icon_proxy,
		                  "AttentionRequested",
		                  NULL, /* params */
		                  G_DBUS_CALL_FLAGS_NONE,
		                  -1, /* timeout */
		                  NULL, /* cancel */
		                  attention_cb,
		                  sm);
		g_dbus_proxy_call(icon_proxy,
		                  "IconShown",
		                  NULL, /* params */
		                  G_DBUS_CALL_FLAGS_NONE,
		                  -1, /* timeout */
		                  NULL, /* cancel */
		                  icon_cb,
		                  sm);
	}

	return;
}

typedef struct _indicator_item_t indicator_item_t;
struct _indicator_item_t {
	GtkWidget * icon;
	GtkWidget * label;
	GtkWidget * right;
};

/* Builds the main image icon using the libindicator helper. */
static GtkImage *
get_icon (IndicatorObject * io)
{
	main_image = GTK_WIDGET(indicator_image_helper("indicator-messages"));
	gtk_widget_show(main_image);

	return GTK_IMAGE(main_image);
}

/* Builds the menu for the indicator */
static GtkMenu *
get_menu (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);
	GtkMenu *menu;

	menu = menu_factory_gtk_new_menu (self->menu, self->actions);
	gtk_widget_show_all (GTK_WIDGET (menu));

	return menu;
}

/* Returns the accessible description of the indicator */
static const gchar *
get_accessible_desc (IndicatorObject * io)
{
	return accessible_desc;
}

/* Returns the name hint of the indicator */
static const gchar *
get_name_hint (IndicatorObject *io)
{
  return PACKAGE;
}

/* Hide the notifications on middle-click over the indicator-messages */
static void
indicator_messages_middle_click (IndicatorObject * io, IndicatorObjectEntry * entry,
                                 guint time, gpointer data)
{
	if (icon_proxy == NULL) {
		return;
	}

	g_dbus_proxy_call(icon_proxy,
	                  "ClearAttention",
	                  NULL, /* params */
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1, /* timeout */
	                  NULL, /* cancel */
	                  NULL,
	                  NULL);

	return;
}
