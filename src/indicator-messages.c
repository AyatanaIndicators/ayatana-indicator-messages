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
#include <glib.h>
#include <gtk/gtk.h>
#include <libdbusmenu-gtk/menu.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libindicator/indicator.h>
INDICATOR_SET_VERSION
INDICATOR_SET_NAME("messages")

#include "dbus-data.h"
#include "messages-service-client.h"

static GtkWidget * main_image = NULL;

#define DESIGN_TEAM_SIZE  design_team_size
static GtkIconSize design_team_size;

static DBusGProxy * icon_proxy = NULL;

static void
attention_changed_cb (DBusGProxy * proxy, gboolean dot, gpointer userdata)
{
	if (dot) {
		gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages-new", DESIGN_TEAM_SIZE);
	} else {
		gtk_image_set_from_icon_name(GTK_IMAGE(main_image), "indicator-messages", DESIGN_TEAM_SIZE);
	}
	return;
}

static void
icon_changed_cb (DBusGProxy * proxy, gboolean hidden, gpointer userdata)
{
	if (hidden) {
		gtk_widget_hide(main_image);
	} else {
		gtk_widget_show(main_image);
	}
	return;
}

static void
watch_cb (DBusGProxy * proxy, GError * error, gpointer userdata)
{
	if (error != NULL) {
		g_warning("Watch failed!  %s", error->message);
		g_error_free(error);
	}
	return;
}

static void
attention_cb (DBusGProxy * proxy, gboolean dot, GError * error, gpointer userdata)
{
	if (error != NULL) {
		g_warning("Unable to get attention status: %s", error->message);
		g_error_free(error);
		return;
	}

	return attention_changed_cb(proxy, dot, userdata);
}

static void
icon_cb (DBusGProxy * proxy, gboolean hidden, GError * error, gpointer userdata)
{
	if (error != NULL) {
		g_warning("Unable to get icon visibility: %s", error->message);
		g_error_free(error);
		return;
	}

	return icon_changed_cb(proxy, hidden, userdata);
}

static gboolean
setup_icon_proxy (gpointer userdata)
{
	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	if (connection == NULL) {
		g_warning("Unable to get session bus");
		return FALSE; /* TRUE? */
	}

	icon_proxy = dbus_g_proxy_new_for_name(connection,
	                                       INDICATOR_MESSAGES_DBUS_NAME,
	                                       INDICATOR_MESSAGES_DBUS_SERVICE_OBJECT,
	                                       INDICATOR_MESSAGES_DBUS_SERVICE_INTERFACE);
	if (icon_proxy == NULL) {
		g_warning("Unable to get messages service interface.");
		return FALSE;
	}
	
	org_ayatana_indicator_messages_service_watch_async(icon_proxy, watch_cb, NULL);

	dbus_g_proxy_add_signal(icon_proxy, "AttentionChanged", G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(icon_proxy,
	                            "AttentionChanged",
	                            G_CALLBACK(attention_changed_cb),
	                            NULL,
	                            NULL);

	dbus_g_proxy_add_signal(icon_proxy, "IconChanged", G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(icon_proxy,
	                            "IconChanged",
	                            G_CALLBACK(icon_changed_cb),
	                            NULL,
	                            NULL);

	org_ayatana_indicator_messages_service_attention_requested_async(icon_proxy, attention_cb, NULL);
	org_ayatana_indicator_messages_service_icon_shown_async(icon_proxy, icon_cb, NULL);

	return FALSE;
}

static gboolean
new_launcher_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
	GtkMenuItem * gmi = GTK_MENU_ITEM(gtk_menu_item_new());

	GtkWidget * vbox = gtk_vbox_new(TRUE, 2);

	GtkWidget * app_label = gtk_label_new(dbusmenu_menuitem_property_get(newitem, LAUNCHER_MENUITEM_PROP_APP_NAME));
	gtk_misc_set_alignment(GTK_MISC(app_label), 0.0, 0.5);
	GtkWidget * dsc_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(dsc_label), 0.05, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL(dsc_label), PANGO_ELLIPSIZE_END);
	gchar * markup = g_markup_printf_escaped("<span font-size=\"smaller\">%s</span>", dbusmenu_menuitem_property_get(newitem, LAUNCHER_MENUITEM_PROP_APP_DESC));
	gtk_label_set_markup(GTK_LABEL(dsc_label), markup);
	g_free(markup);

	gtk_box_pack_start(GTK_BOX(vbox), app_label, FALSE, FALSE, 0);
	gtk_widget_show(app_label);
	gtk_box_pack_start(GTK_BOX(vbox), dsc_label, FALSE, FALSE, 0);
	gtk_widget_show(dsc_label);

	gtk_container_add(GTK_CONTAINER(gmi), GTK_WIDGET(vbox));
	gtk_widget_show(GTK_WIDGET(vbox));

	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, gmi, parent);

	return TRUE;
}

GtkLabel *
get_label (void)
{
	return NULL;
}

GtkImage *
get_icon (void)
{
	design_team_size = gtk_icon_size_register("design-team-size", 22, 22);

	main_image = gtk_image_new_from_icon_name("indicator-messages", DESIGN_TEAM_SIZE);
	gtk_widget_show(main_image);

	return GTK_IMAGE(main_image);
}

GtkMenu *
get_menu (void)
{
	guint returnval = 0;
	GError * error = NULL;

	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	DBusGProxy * proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_start_service_by_name (proxy, INDICATOR_MESSAGES_DBUS_NAME, 0, &returnval, &error)) {
		g_error("Unable to send message to DBus to start service: %s", error != NULL ? error->message : "(NULL error)" );
		g_error_free(error);
		return NULL;
	}

	if (returnval != DBUS_START_REPLY_SUCCESS && returnval != DBUS_START_REPLY_ALREADY_RUNNING) {
		g_error("Return value isn't indicative of success: %d", returnval);
		return NULL;
	}

	g_idle_add(setup_icon_proxy, NULL);

	DbusmenuGtkMenu * menu = dbusmenu_gtkmenu_new(INDICATOR_MESSAGES_DBUS_NAME, INDICATOR_MESSAGES_DBUS_OBJECT);
	DbusmenuGtkClient * client = dbusmenu_gtkmenu_get_client(menu);

	dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), LAUNCHER_MENUITEM_TYPE, new_launcher_item);

	return GTK_MENU(menu);
}

