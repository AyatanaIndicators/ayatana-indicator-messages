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

#include "dbus-data.h"

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
static GtkImage * get_image               (IndicatorObject * io);
static GtkMenu * get_menu                 (IndicatorObject * io);
static const gchar * get_accessible_desc  (IndicatorObject * io);
static const gchar * get_name_hint        (IndicatorObject * io);
static void update_root_item              (IndicatorMessages * self);
static void update_menu                   (IndicatorMessages *self);
static void menu_items_changed            (GMenuModel *menu,
                                           gint        position,
                                           gint        removed,
                                           gint        added,
                                           gpointer    user_data);

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
	GDBusConnection *bus;
	GError *error = NULL;

	/* Default values */
	self->service = NULL;

	/* Complex stuff */
	self->service = indicator_service_manager_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);

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

	g_signal_connect (self->menu, "items-changed", G_CALLBACK (menu_items_changed), self);

	self->image = g_object_ref_sink (gtk_image_new ());
	gtk_widget_show (self->image);
	update_root_item (self);

	self->menu_wrapper = g_menu_new ();
	update_menu (self);

	g_object_unref (bus);
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

static GtkImage *
get_image (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);

	return GTK_IMAGE (self->image);
}

static GtkMenu *
get_menu (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);
	GtkWidget *menu;

	menu = gtk_menu_new_from_model (G_MENU_MODEL (self->menu_wrapper));
	gtk_widget_insert_action_group (menu, get_name_hint (io), self->actions);

	return GTK_MENU (menu);
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

static void
update_root_item (IndicatorMessages * self)
{
	const gchar *icon_name;

	if (g_menu_model_get_n_items (self->menu) == 0)
		return;

	g_menu_model_get_item_attribute (self->menu, 0, INDICATOR_MENU_ATTRIBUTE_ICON_NAME,
					 "&s", &icon_name);

	g_free (self->accessible_desc);

	g_menu_model_get_item_attribute (self->menu, 0, INDICATOR_MENU_ATTRIBUTE_ACCESSIBLE_DESCRIPTION,
					 "s", &self->accessible_desc);
	indicator_messages_accessible_desc_updated (self);

	gtk_image_set_from_icon_name (GTK_IMAGE (self->image), icon_name, GTK_ICON_SIZE_MENU);
}

static void
update_menu (IndicatorMessages *self)
{
	GMenuModel *popup;
	GMenuItem *item;

	if (g_menu_model_get_n_items (self->menu) == 0)
		return;

	popup = g_menu_model_get_item_link (self->menu, 0, G_MENU_LINK_SUBMENU);
	if (popup == NULL)
		return;

	if (g_menu_model_get_n_items (G_MENU_MODEL (self->menu_wrapper)) == 1)
		g_menu_remove (self->menu_wrapper, 0);

	item = g_menu_item_new_section (NULL, popup);
	g_menu_item_set_attribute (item, "action-namespace",
				   "s", get_name_hint (INDICATOR_OBJECT (self)));
	g_menu_append_item (self->menu_wrapper, item);

	g_object_unref (item);
	g_object_unref (popup);
}

static void
menu_items_changed (GMenuModel *menu,
                    gint        position,
                    gint        removed,
                    gint        added,
                    gpointer    user_data)
{
	IndicatorMessages *self = user_data;

	if (position == 0) {
		update_root_item (self);
		update_menu (self);
	}
}
