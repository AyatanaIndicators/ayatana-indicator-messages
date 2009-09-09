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

#include <glib/gi18n.h>
#include <libdbusmenu-glib/client.h>
#include <libindicate/indicator.h>
#include <libindicate/indicator-messages.h>
#include <libindicate/listener.h>
#include "im-menu-item.h"
#include "dbus-data.h"

enum {
	TIME_CHANGED,
	ATTENTION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _ImMenuItemPrivate ImMenuItemPrivate;

struct _ImMenuItemPrivate
{
	IndicateListener *           listener;
	IndicateListenerServer *      server;
	IndicateListenerIndicator *  indicator;

	glong seconds;
	gchar * count;
	gulong indicator_changed;
	gboolean attention;
	gboolean show;

	guint time_update_min;
};

#define IM_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_MENU_ITEM_TYPE, ImMenuItemPrivate))

/* Prototypes */
static void im_menu_item_class_init (ImMenuItemClass *klass);
static void im_menu_item_init       (ImMenuItem *self);
static void im_menu_item_dispose    (GObject *object);
static void im_menu_item_finalize   (GObject *object);
static void sender_cb               (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     gchar * propertydata,
                                     gpointer data);
static void time_cb                 (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     GTimeVal * propertydata,
                                     gpointer data);
static void icon_cb                 (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     gchar * propertydata,
                                     gpointer data);
static void activate_cb             (ImMenuItem * self,
                                     gpointer data);
static void indicator_modified_cb   (IndicateListener * listener,
                                     IndicateListenerServer * server,
                                     IndicateListenerIndicator * indicator,
                                     gchar * property,
                                     ImMenuItem * self);

G_DEFINE_TYPE (ImMenuItem, im_menu_item, DBUSMENU_TYPE_MENUITEM);

static void
im_menu_item_class_init (ImMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ImMenuItemPrivate));

	object_class->dispose = im_menu_item_dispose;
	object_class->finalize = im_menu_item_finalize;

	signals[TIME_CHANGED] =  g_signal_new(IM_MENU_ITEM_SIGNAL_TIME_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (ImMenuItemClass, time_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__LONG,
	                                      G_TYPE_NONE, 1, G_TYPE_LONG);
	signals[ATTENTION_CHANGED] =  g_signal_new(IM_MENU_ITEM_SIGNAL_ATTENTION_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (ImMenuItemClass, attention_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__BOOLEAN,
	                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	return;
}

static void
im_menu_item_init (ImMenuItem *self)
{
	g_debug("Building new IM Menu Item");
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = NULL;
	priv->server = NULL;
	priv->indicator = NULL;

	priv->seconds = 0;

	return;
}

static void
im_menu_item_dispose (GObject *object)
{
	G_OBJECT_CLASS (im_menu_item_parent_class)->dispose (object);

	ImMenuItem * self = IM_MENU_ITEM(object);
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	if (priv->time_update_min != 0) {
		g_source_remove(priv->time_update_min);
	}

	g_signal_handler_disconnect(priv->listener, priv->indicator_changed);
	priv->indicator_changed = 0;

	return;
}

static void
im_menu_item_finalize (GObject *object)
{
	G_OBJECT_CLASS (im_menu_item_parent_class)->finalize (object);
}

/* Call back for getting icon data.  It just passes it along
   to the indicator so that it can visualize it.  Not our problem. */
static void
icon_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(data), INDICATOR_MENUITEM_PROP_ICON, propertydata);
	return;
}

/* This function takes the time and turns it into the appropriate
   string to put on the right side of the menu item.  Of course it
   doesn't do that if there is a count set.  If there's a count then
   it gets that space. */
static void
update_time (ImMenuItem * self)
{
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	if (priv->count != NULL) {
		return;
	}
	
	gchar * timestring = NULL;

	GTimeVal current_time;
	g_get_current_time(&current_time);

	guint elapsed_seconds = current_time.tv_sec - priv->seconds;
	guint elapsed_minutes = elapsed_seconds / 60;

	if (elapsed_seconds % 60 > 55) {
		/* We're using fuzzy timers, so we need fuzzy comparisons */
		elapsed_minutes += 1;
	}

	if (elapsed_minutes < 60) {
		timestring = g_strdup_printf(ngettext("%d m", "%d m", elapsed_minutes), elapsed_minutes);
	} else {
		guint elapsed_hours = elapsed_minutes / 60;

		if (elapsed_minutes % 60 > 55) {
			/* We're using fuzzy timers, so we need fuzzy comparisons */
			elapsed_hours += 1;
		}

		timestring = g_strdup_printf(ngettext("%d h", "%d h", elapsed_hours), elapsed_hours);
	}

	if (timestring != NULL) {
		dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), INDICATOR_MENUITEM_PROP_RIGHT, timestring);
		g_free(timestring);
	}

	return;
}

/* This is a wrapper around update_time that matches the prototype
   needed to make this a timer callback.  Silly. */
static gboolean
time_update_cb (gpointer data)
{
	ImMenuItem * self = IM_MENU_ITEM(data);

	update_time(self);

	return TRUE;
}

/* Yet another time function.  This one takes the time as formated as
   we get it from libindicate and turns it into the seconds that we're
   looking for.  It should only be called once at the init with a new
   indicator and again when the value changes. */
static void
time_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, GTimeVal * propertydata, gpointer data)
{
	g_debug("Got Time info");
	ImMenuItem * self = IM_MENU_ITEM(data);
	if (self == NULL) {
		g_error("Menu Item callback called without a menu item");
		return;
	}

	if (property == NULL || g_strcmp0(property, "time")) {
		g_warning("Time callback called without being sent the time.");
		return;
	}

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	priv->seconds = propertydata->tv_sec;

	update_time(self);

	if (priv->time_update_min == 0) {
		priv->time_update_min = g_timeout_add_seconds(60, time_update_cb, self);
	}

	g_signal_emit(G_OBJECT(self), signals[TIME_CHANGED], 0, priv->seconds, TRUE);

	return;
}

/* Callback from libindicate that is for getting the sender information
   on a particular indicator. */
static void
sender_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	g_debug("Got Sender Information: %s", propertydata);
	ImMenuItem * self = IM_MENU_ITEM(data);

	/* Our data should be right */
	g_return_if_fail(self != NULL);
	/* We should have a property name */
	g_return_if_fail(property != NULL);
	/* The Property should be sender or name */
	g_return_if_fail(!g_strcmp0(property, "sender") || !g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_NAME));

	/* We might get the sender variable returning a
	   null string as it doesn't exist on newer clients
	   but we don't want to listen to that. */
	if (!g_strcmp0(property, "sender") && propertydata[0] == '\0') {
		return;
	}

	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), INDICATOR_MENUITEM_PROP_LABEL, propertydata);

	return;
}

/* Callback saying that the count is updated, we need to either put
   that on the menu item or just remove it if the count is gone.  If
   that's the case we can update time. */
static void
count_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	g_debug("Got Count Information");
	ImMenuItem * self = IM_MENU_ITEM(data);

	/* Our data should be right */
	g_return_if_fail(self != NULL);
	/* We should have a property name */
	g_return_if_fail(property != NULL);
	/* The Property should be count */
	g_return_if_fail(!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_COUNT));

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	if (propertydata == NULL || propertydata[0] == '\0') {
		/* The count is either being unset or it was never
		   set in the first place. */
		if (priv->count != NULL) {
			g_free(priv->count);
			priv->count = NULL;
			update_time(self);
		}
		return;
	}

	if (priv->count != NULL) {
		g_free(priv->count);
	}

	priv->count = g_strdup_printf("(%s)", propertydata);
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), INDICATOR_MENUITEM_PROP_RIGHT, priv->count);

	return;
}

/* This is getting the attention variable that's looking at whether
   this indicator should be calling for attention or not.  If we are,
   we need to signal that. */
static void
attention_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, gchar * propertydata, gpointer data)
{
	g_debug("Got Attention Information");
	ImMenuItem * self = IM_MENU_ITEM(data);

	/* Our data should be right */
	g_return_if_fail(self != NULL);
	/* We should have a property name */
	g_return_if_fail(property != NULL);
	/* The Property should be count */
	g_return_if_fail(!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION));

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	gboolean wantit;
	if (propertydata == NULL || propertydata[0] == '\0' || !g_strcmp0(propertydata, "false")) {
		wantit = FALSE;
	} else {
		wantit = TRUE;
	}

	if (priv->attention != wantit) {
		priv->attention = wantit;
		g_signal_emit(G_OBJECT(self), signals[ATTENTION_CHANGED], 0, wantit, TRUE);
	}

	return;
}

/* Callback when the item gets clicked on from the Messaging Menu */
static void
activate_cb (ImMenuItem * self, gpointer data)
{
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	indicate_listener_display(priv->listener, priv->server, priv->indicator);
}

/* Callback when a property gets modified.  It figures out which one
   got modified and notifies the appropriate person. */
void
indicator_modified_cb (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator, gchar * property, ImMenuItem * self)
{
	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	/* Not meant for us */
	if (INDICATE_LISTENER_INDICATOR_ID(indicator) != INDICATE_LISTENER_INDICATOR_ID(priv->indicator)) return;
	if (server != priv->server) return;

	/* Determine which property has been changed and request the
	   value go to the appropriate callback. */
	if (!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_NAME)) {
		indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_NAME, sender_cb, self);	
	} else if (!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_TIME)) {
		indicate_listener_get_property_time(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_TIME, time_cb, self);	
	} else if (!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_ICON)) {
		indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_ICON, icon_cb, self);	
	} else if (!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_COUNT)) {
		indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_COUNT, count_cb, self);	
	} else if (!g_strcmp0(property, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION)) {
		indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, attention_cb, self);	
	} else if (!g_strcmp0(property, "sender")) {
		/* This is a compatibility string with v1 and should be removed */
		g_debug("Indicator is using 'sender' property which is a v1 string.");
		indicate_listener_get_property(listener, server, indicator, "sender", sender_cb, self);	
	}
	
	return;
}

ImMenuItem *
im_menu_item_new (IndicateListener * listener, IndicateListenerServer * server, IndicateListenerIndicator * indicator)
{
	ImMenuItem * self = g_object_new(IM_MENU_ITEM_TYPE, NULL);

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(self);

	priv->listener = listener;
	priv->server = server;
	priv->indicator = indicator;
	priv->count = NULL;
	priv->time_update_min = 0;
	priv->attention = FALSE;
	priv->show = TRUE;

	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), "type", INDICATOR_MENUITEM_TYPE);

	indicate_listener_displayed(listener, server, indicator, TRUE);

	indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_NAME, sender_cb, self);	
	indicate_listener_get_property_time(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_TIME, time_cb, self);	
	indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_ICON, icon_cb, self);	
	indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_COUNT, count_cb, self);	
	indicate_listener_get_property(listener, server, indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, attention_cb, self);	
	indicate_listener_get_property(listener, server, indicator, "sender", sender_cb, self);	

	g_signal_connect(G_OBJECT(self), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(activate_cb), NULL);
	priv->indicator_changed = g_signal_connect(G_OBJECT(listener), INDICATE_LISTENER_SIGNAL_INDICATOR_MODIFIED, G_CALLBACK(indicator_modified_cb), self);

	return self;
}

/* Gets the number of seconds for the creator
   of this item. */
glong
im_menu_item_get_seconds (ImMenuItem * menuitem)
{
	g_return_val_if_fail(IS_IM_MENU_ITEM(menuitem), 0);

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(menuitem);
	return priv->seconds;
}

/* Gets whether or not this indicator item is
   asking for attention or not. */
gboolean
im_menu_item_get_attention (ImMenuItem * menuitem)
{
	g_return_val_if_fail(IS_IM_MENU_ITEM(menuitem), FALSE);

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(menuitem);
	return priv->attention;
}

/* This takes care of items that need to be hidden, this is
   usually because they go over the count of allowed indicators.
   Which is more than a little bit silly.  We shouldn't do that.
   But we need to enforce it to save users against bad apps. */
void
im_menu_item_show (ImMenuItem * menuitem, gboolean show)
{
	g_return_if_fail(IS_IM_MENU_ITEM(menuitem));

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(menuitem);

	if (priv->show == show) {
		return;
	}

	priv->show = show;
	/* Tell the app what we're doing to it.  If it's being
	   punished it needs to know about it. */
	indicate_listener_displayed(priv->listener, priv->server, priv->indicator, priv->show);
	if (priv->attention) {
		/* If we were asking for attention we can ask for it
		   again if we're being shown, otherwise no. */
		g_signal_emit(G_OBJECT(menuitem), signals[ATTENTION_CHANGED], 0, priv->show, TRUE);
	}
	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(menuitem), DBUSMENU_MENUITEM_PROP_VISIBLE, priv->show ? "true" : "false");

	return;
}

/* Check to see if this item is shown.  Accessor for the
   internal variable. */
gboolean
im_menu_item_shown (ImMenuItem * menuitem)
{
	g_return_val_if_fail(IS_IM_MENU_ITEM(menuitem), FALSE);

	ImMenuItemPrivate * priv = IM_MENU_ITEM_GET_PRIVATE(menuitem);

	return priv->show;
}
