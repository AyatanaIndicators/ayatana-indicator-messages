/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>
    Lars Uebernickel <lars.uebernickel@canonical.com>

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
#include <libindicator/indicator-service-manager.h>

#include "dbus-data.h"

#include "ido-menu-item.h"
#include "im-app-menu-item.h"
#include "im-source-menu-item.h"

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
};

struct _IndicatorMessages {
	IndicatorObject parent;
	IndicatorServiceManager * service;
	GActionGroup *actions;
	GMenu *menu_wrapper;
	GMenuModel *menu;
	GtkWidget *image;
	GtkWidget *gtkmenu;
	gchar *accessible_desc;
};

GType indicator_messages_get_type (void);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_MESSAGES_TYPE)

/* Prototypes */
static void indicator_messages_class_init (IndicatorMessagesClass *klass);
static void indicator_messages_init       (IndicatorMessages *self);
static void indicator_messages_dispose    (GObject *object);
static void indicator_messages_finalize   (GObject *object);
static void service_connection_changed    (IndicatorServiceManager *sm,
					   gboolean connected,
					   gpointer user_data);
static GtkImage * get_image               (IndicatorObject * io);
static GtkMenu * get_menu                 (IndicatorObject * io);
static const gchar * get_accessible_desc  (IndicatorObject * io);
static const gchar * get_name_hint        (IndicatorObject * io);
static void menu_items_changed            (GMenuModel *menu,
                                           gint        position,
                                           gint        removed,
                                           gint        added,
                                           gpointer    user_data);
static void messages_state_changed        (GActionGroup *action_group,
                                           gchar        *action_name,
                                           GVariant     *value,
                                           gpointer      user_data);
static void indicator_messages_add_toplevel_menu (IndicatorMessages *self);

G_DEFINE_TYPE (IndicatorMessages, indicator_messages, INDICATOR_OBJECT_TYPE);

/* Initialize the one-timers */
static void
indicator_messages_class_init (IndicatorMessagesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_messages_dispose;
	object_class->finalize = indicator_messages_finalize;

	IndicatorObjectClass * io_class = INDICATOR_OBJECT_CLASS(klass);

	io_class->get_image = get_image;
	io_class->get_menu = get_menu;
	io_class->get_accessible_desc = get_accessible_desc;
	io_class->get_name_hint = get_name_hint;
}

/* Build up our per-instance variables */
static void
indicator_messages_init (IndicatorMessages *self)
{
	self->service = indicator_service_manager_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);
	g_signal_connect (self->service, "connection-change",
			  G_CALLBACK (service_connection_changed), self);

	self->menu_wrapper = g_menu_new ();
	self->gtkmenu = gtk_menu_new_from_model (G_MENU_MODEL (self->menu_wrapper));
	g_object_ref_sink (self->gtkmenu);

	self->image = g_object_ref_sink (gtk_image_new ());

	/* make sure custom menu item types are registered (so that
         * gtk_model_new_from_menu can pick them up */
	ido_menu_item_get_type ();
	im_app_menu_item_get_type ();
	im_source_menu_item_get_type ();
}

/* Unref stuff */
static void
indicator_messages_dispose (GObject *object)
{
	IndicatorMessages * self = INDICATOR_MESSAGES(object);
	g_return_if_fail(self != NULL);

	g_clear_object (&self->service);
	g_clear_object (&self->menu_wrapper);
	g_clear_object (&self->actions);
	g_clear_object (&self->menu);
	g_clear_object (&self->gtkmenu);
	g_clear_object (&self->image);

	G_OBJECT_CLASS (indicator_messages_parent_class)->dispose (object);
	return;
}

/* Destory all memory users */
static void
indicator_messages_finalize (GObject *object)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (object);

	g_free (self->accessible_desc);

	G_OBJECT_CLASS (indicator_messages_parent_class)->finalize (object);
	return;
}



/* Functions */

static void service_connection_changed (IndicatorServiceManager *sm,
				        gboolean connected,
				        gpointer user_data)
{
	IndicatorMessages *self = user_data;
	GDBusConnection *bus;
	GError *error = NULL;

	if (self->actions != NULL) {
		g_signal_handlers_disconnect_by_func (self->actions, messages_state_changed, self);
		g_clear_object (&self->actions);
	}
	if (self->menu != NULL) {
		g_signal_handlers_disconnect_by_func (self->menu, menu_items_changed, self);
		g_clear_object (&self->menu);
	}
	if (g_menu_model_get_n_items (G_MENU_MODEL (self->menu_wrapper)) == 1)
		g_menu_remove (self->menu_wrapper, 0);

	if (connected == FALSE)
		return;

	bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (!bus) {
		g_warning ("error connecting to the session bus: %s", error->message);
		g_error_free (error);
		return;
	}

	self->actions = G_ACTION_GROUP (g_dbus_action_group_get (bus,
								 INDICATOR_MESSAGES_DBUS_NAME,
								 INDICATOR_MESSAGES_DBUS_OBJECT));
	gtk_widget_insert_action_group (self->gtkmenu, 
					get_name_hint (INDICATOR_OBJECT (self)),
					self->actions);
	g_signal_connect (self->actions, "action-state-changed::messages",
			  G_CALLBACK (messages_state_changed), self);

	self->menu = G_MENU_MODEL (g_dbus_menu_model_get (bus,
							  INDICATOR_MESSAGES_DBUS_NAME,
							  INDICATOR_MESSAGES_DBUS_OBJECT));
	g_signal_connect (self->menu, "items-changed", G_CALLBACK (menu_items_changed), self);

	if (g_menu_model_get_n_items (self->menu) == 1)
		indicator_messages_add_toplevel_menu (self);
	else
		indicator_object_set_visible (INDICATOR_OBJECT (self), FALSE);

	g_object_unref (bus);
}

static GtkImage *
get_image (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);

	gtk_widget_show (self->image);
	return GTK_IMAGE (self->image);
}

static GtkMenu *
get_menu (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);

	return GTK_MENU (self->gtkmenu);
}

static const gchar *
get_accessible_desc (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);
	return self->accessible_desc;
}

static const gchar *
get_name_hint (IndicatorObject *io)
{
  return PACKAGE;
}

static void
indicator_messages_accessible_desc_updated (IndicatorMessages *self)
{
	GList *entries;

	entries = indicator_object_get_entries (INDICATOR_OBJECT (self));
	g_return_if_fail (entries != NULL);

	g_signal_emit_by_name (self, INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE, entries->data);

	g_list_free (entries);
}

static GIcon *
g_menu_model_get_item_attribute_icon (GMenuModel  *menu,
				      gint         index,
				      const gchar *attribute)
{
	gchar *iconstr;
	GIcon *icon = NULL;

	if (g_menu_model_get_item_attribute (menu, index, attribute, "s", &iconstr)) {
		GError *error;

		icon = g_icon_new_for_string (iconstr, &error);
		if (icon == NULL) {
			g_warning ("unable to load icon: %s", error->message);
			g_error_free (error);
		}

		g_free (iconstr);
	}

	return icon;
}

static void
indicator_messages_add_toplevel_menu (IndicatorMessages *self)
{
	GIcon *icon;
	GMenuModel *popup;

	indicator_object_set_visible (INDICATOR_OBJECT (self), TRUE);

	icon = g_menu_model_get_item_attribute_icon (self->menu, 0, "x-canonical-icon");
	if (icon) {
		gtk_image_set_from_gicon (GTK_IMAGE (self->image), icon, GTK_ICON_SIZE_MENU);
		g_object_unref (icon);
	}

	g_free (self->accessible_desc);
	self->accessible_desc = NULL;
	if (g_menu_model_get_item_attribute (self->menu, 0, "x-canonical-accessible-description",
					     "s", &self->accessible_desc)) {
		indicator_messages_accessible_desc_updated (self);
	}

	popup = g_menu_model_get_item_link (self->menu, 0, G_MENU_LINK_SUBMENU);
	if (popup) {
		GMenuItem *item;

		item = g_menu_item_new_section (NULL, popup);
		g_menu_item_set_attribute (item, "action-namespace",
					   "s", get_name_hint (INDICATOR_OBJECT (self)));
		g_menu_append_item (self->menu_wrapper, item);

		g_object_unref (item);
		g_object_unref (popup);
	}
}

static void
menu_items_changed (GMenuModel *menu,
                    gint        position,
                    gint        removed,
                    gint        added,
                    gpointer    user_data)
{
	IndicatorMessages *self = user_data;

	g_return_if_fail (position == 0);

	if (added == 1) {
		indicator_messages_add_toplevel_menu (self);
	}
	else if (removed == 1) {
		g_menu_remove (self->menu_wrapper, 0);
		indicator_object_set_visible (INDICATOR_OBJECT (self), FALSE);
	}
}

static void
messages_state_changed (GActionGroup *action_group,
                        gchar        *action_name,
                        GVariant     *value,
                        gpointer      user_data)
{
	IndicatorMessages *self = user_data;

	g_return_if_fail (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN));

	if (g_variant_get_boolean (value))
		gtk_image_set_from_icon_name (GTK_IMAGE (self->image), "indicator-messages-new", GTK_ICON_SIZE_MENU);
	else
		gtk_image_set_from_icon_name (GTK_IMAGE (self->image), "indicator-messages", GTK_ICON_SIZE_MENU);
}
