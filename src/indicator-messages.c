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
	GMenuModel *menu;
};

GType indicator_messages_get_type (void);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_MESSAGES_TYPE)

/* Globals */
static const gchar *              accessible_desc = NULL;
static IndicatorObject *          indicator = NULL;

/* Prototypes */
static void indicator_messages_class_init (IndicatorMessagesClass *klass);
static void indicator_messages_init       (IndicatorMessages *self);
static void indicator_messages_dispose    (GObject *object);
static void indicator_messages_finalize   (GObject *object);
static GMenuModel * get_menu_model        (IndicatorObject * io);
static GActionGroup * get_actions         (IndicatorObject * io);
static const gchar * get_accessible_desc  (IndicatorObject * io);
static const gchar * get_name_hint        (IndicatorObject * io);

G_DEFINE_TYPE (IndicatorMessages, indicator_messages, INDICATOR_OBJECT_TYPE);

/* Initialize the one-timers */
static void
indicator_messages_class_init (IndicatorMessagesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_messages_dispose;
	object_class->finalize = indicator_messages_finalize;

	IndicatorObjectClass * io_class = INDICATOR_OBJECT_CLASS(klass);

	io_class->get_menu_model = get_menu_model;
	io_class->get_actions = get_actions;
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

typedef struct _indicator_item_t indicator_item_t;
struct _indicator_item_t {
	GtkWidget * icon;
	GtkWidget * label;
	GtkWidget * right;
};

/* Builds the menu for the indicator */
static GMenuModel *
get_menu_model (IndicatorObject * io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);
	return self->menu;
}

static GActionGroup *
get_actions (IndicatorObject *io)
{
	IndicatorMessages *self = INDICATOR_MESSAGES (io);
	return self->actions;
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
