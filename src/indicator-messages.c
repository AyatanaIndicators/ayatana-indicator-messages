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
#include <gtk/gtk.h>
#include <libdbusmenu-gtk/menu.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libindicator/indicator.h>
INDICATOR_SET_VERSION
INDICATOR_SET_NAME("messages")

#include "dbus-data.h"

static GtkWidget * main_image;

#define DESIGN_TEAM_SIZE  design_team_size
static GtkIconSize design_team_size;

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

	/* Need a proxy here to figure out when the icon changes */

	return GTK_IMAGE(main_image);
}

GtkMenu *
get_menu (void)
{
	guint returnval = 0;
	GError * error = NULL;

	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	DBusGProxy * proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	org_freedesktop_DBus_start_service_by_name (proxy, INDICATOR_MESSAGES_DBUS_NAME, 0, &returnval, &error);

	return GTK_MENU(dbusmenu_gtkmenu_new(INDICATOR_MESSAGES_DBUS_NAME, INDICATOR_MESSAGES_DBUS_OBJECT));
}

