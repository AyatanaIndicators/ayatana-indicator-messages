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
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3, 0, 0)
#include <libdbusmenu-gtk3/menu.h>
#include <libdbusmenu-gtk3/menuitem.h>
#else
#include <libdbusmenu-gtk/menu.h>
#include <libdbusmenu-gtk/menuitem.h>
#endif

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-image-helper.h>
#include <libindicator/indicator-service-manager.h>

#include "dbus-data.h"
#include "gen-messages-service.xml.h"

#define INDICATOR_MESSAGES_TYPE            (indicator_messages_get_type ())
#define INDICATOR_MESSAGES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_MESSAGES_TYPE, IndicatorMessages))
#define INDICATOR_MESSAGES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_MESSAGES_TYPE, IndicatorMessagesClass))
#define IS_INDICATOR_MESSAGES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_MESSAGES_TYPE))
#define IS_INDICATOR_MESSAGES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_MESSAGES_TYPE))
#define INDICATOR_MESSAGES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_MESSAGES_TYPE, IndicatorMessagesClass))

#define M_PI 3.1415926535897932384626433832795028841971693993751

#define RIGHT_LABEL_FONT_SIZE 12
#define RIGHT_LABEL_RADIUS 20

typedef struct _IndicatorMessages      IndicatorMessages;
typedef struct _IndicatorMessagesClass IndicatorMessagesClass;

struct _IndicatorMessagesClass {
	IndicatorObjectClass parent_class;
	void    (*update_a11y_desc) (IndicatorServiceManager * service, gpointer * user_data);
};

struct _IndicatorMessages {
	IndicatorObject parent;
	IndicatorServiceManager * service;
};

GType indicator_messages_get_type (void);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_MESSAGES_TYPE)

/* Globals */
static GtkWidget * main_image = NULL;
static GDBusProxy * icon_proxy = NULL;
static GtkSizeGroup * indicator_right_group = NULL;
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
	/* Default values */
	self->service = NULL;

	/* Complex stuff */
	self->service = indicator_service_manager_new_version(INDICATOR_MESSAGES_DBUS_NAME, 1);
	g_signal_connect(self->service, INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE, G_CALLBACK(connection_change), self);

	indicator = INDICATOR_OBJECT(self);

	return;
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

/* Sets the icon when it changes. */
static void
application_icon_change_cb (DbusmenuMenuitem * mi, gchar * prop, GVariant * value, gpointer user_data)
{
	if (!g_strcmp0(prop, APPLICATION_MENUITEM_PROP_ICON)) {
		/* Set the main icon */
		if (GTK_IS_IMAGE(user_data)) {
			gtk_image_set_from_icon_name(GTK_IMAGE(user_data), g_variant_get_string(value, NULL), GTK_ICON_SIZE_MENU);
		}
	}

	return;
}

/* Sets the label when it changes. */
static void
application_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GVariant * value, gpointer user_data)
{
	if (!g_strcmp0(prop, APPLICATION_MENUITEM_PROP_NAME)) {
		/* Set the main label */
		if (GTK_IS_LABEL(user_data)) {
			gtk_label_set_text(GTK_LABEL(user_data), g_variant_get_string(value, NULL));
		}
	}

	if (!g_strcmp0(prop, APPLICATION_MENUITEM_PROP_RUNNING)) {
		/* TODO: should hide/show the triangle live if the menu was open.
		   In practice, this is rarely needed. */
	}

	return;
}

/* Draws a triangle on the left, using fg[STATE_TYPE] color. */
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean
application_triangle_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
#else
static gboolean
application_triangle_draw_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	cairo_t *cr;
#endif
	GtkAllocation allocation;
	GtkStyle *style;

	int x, y, arrow_width, arrow_height;

	if (!GTK_IS_WIDGET (widget)) return FALSE;
	if (!DBUSMENU_IS_MENUITEM (data)) return FALSE;

	/* render the triangle indicator only if the application is running */
	if (! dbusmenu_menuitem_property_get_bool (DBUSMENU_MENUITEM(data), APPLICATION_MENUITEM_PROP_RUNNING))
		return FALSE;;

	/* get style */
	style = gtk_widget_get_style (widget);

	/* set arrow position / dimensions */
	arrow_width = 5; /* the pixel-based reference triangle is 5x9 */
	arrow_height = 9;
	gtk_widget_get_allocation (widget, &allocation);
#if GTK_CHECK_VERSION(3, 0, 0)
	x = 0;
	y = allocation.height/2.0 - (double)arrow_height/2.0;
#else
	x = allocation.x;
	y = allocation.y + allocation.height/2.0 - (double)arrow_height/2.0;
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_save (cr);
#else
	/* initialize cairo drawing area */
	cr = (cairo_t*) gdk_cairo_create (gtk_widget_get_window (widget));
#endif

	/* set line width */
	cairo_set_line_width (cr, 1.0);

	/* cairo drawing code */
	cairo_move_to (cr, x, y);
	cairo_line_to (cr, x, y + arrow_height);
	cairo_line_to (cr, x + arrow_width, y + (double)arrow_height/2.0);
	cairo_close_path (cr);
	cairo_set_source_rgb (cr, style->fg[gtk_widget_get_state(widget)].red/65535.0,
	                          style->fg[gtk_widget_get_state(widget)].green/65535.0,
	                          style->fg[gtk_widget_get_state(widget)].blue/65535.0);
	cairo_fill (cr);

#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_restore (cr);
#else
	/* remember to destroy cairo context to avoid leaks */
	cairo_destroy (cr);
#endif

	return FALSE;
}

/* Custom function to draw rounded rectangle with max radius */
static void
custom_cairo_rounded_rectangle (cairo_t *cr,
                                double x, double y, double w, double h)
{
	double radius = MIN (w/2.0, h/2.0);

	cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+w-radius, y+radius, radius, M_PI*1.5, M_PI*2);
	cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, M_PI*0.5);
	cairo_arc (cr, x+radius,   y+h-radius, radius, M_PI*0.5, M_PI);
	cairo_arc (cr, x+radius,   y+radius,   radius, M_PI, M_PI*1.5);
}

/* Draws a rounded rectangle with text inside. */
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean
numbers_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
#else
static gboolean
numbers_draw_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	cairo_t *cr;
#endif
	GtkAllocation allocation;
	GtkStyle *style;
	double x, y, w, h;
	PangoLayout * layout;
	gint font_size = RIGHT_LABEL_FONT_SIZE;

	if (!GTK_IS_WIDGET (widget)) return FALSE;

	/* get style */
	style = gtk_widget_get_style (widget);

	/* set arrow position / dimensions */
	gtk_widget_get_allocation (widget, &allocation);
#if GTK_CHECK_VERSION(3, 0, 0)
	x = 0;
	y = 0;
#else
	x = allocation.x;
	y = allocation.y;
#endif
	w = allocation.width;
	h = allocation.height;

	layout = gtk_label_get_layout (GTK_LABEL(widget));

	/* This does not work, don't ask me why but font_size is 0.
	 * I wanted to use a dynamic font size to adjust the padding on left/right edges
	 * of the rounded rectangle. Andrea Cimitan */
	/* const PangoFontDescription * font_description = pango_layout_get_font_description (layout);
	font_size = pango_font_description_get_size (font_description); */

#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_save (cr);
#else
	/* initialize cairo drawing area */
	cr = (cairo_t*) gdk_cairo_create (gtk_widget_get_window (widget));
#endif

	/* set line width */
	cairo_set_line_width (cr, 1.0);

	cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

	/* cairo drawing code */
	custom_cairo_rounded_rectangle (cr, x - font_size/2.0, y, w + font_size, h);

	cairo_set_source_rgba (cr, style->fg[gtk_widget_get_state(widget)].red/65535.0,
	                           style->fg[gtk_widget_get_state(widget)].green/65535.0,
	                           style->fg[gtk_widget_get_state(widget)].blue/65535.0, 0.5);

	cairo_move_to (cr, x, y);
	pango_cairo_layout_path (cr, layout);
	cairo_fill (cr);

#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_restore (cr);
#else
	/* remember to destroy cairo context to avoid leaks */
	cairo_destroy (cr);
#endif

	return TRUE;
}

/* Builds a menu item representing a running application in the
   messaging menu */
static gboolean
new_application_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client, gpointer user_data)
{
	g_debug ("%s (\"%s\")", __func__, dbusmenu_menuitem_property_get(newitem, APPLICATION_MENUITEM_PROP_NAME));

	GtkMenuItem * gmi = GTK_MENU_ITEM(gtk_image_menu_item_new());
	gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(gmi), TRUE);

	gint padding = 4;
	gtk_widget_style_get(GTK_WIDGET(gmi), "horizontal-padding", &padding, NULL);

	GtkWidget * hbox = gtk_hbox_new(FALSE, 0);

	/* Set the minimum size, we always want it to take space */
	gint width, height;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);

	GtkWidget * icon = gtk_image_new_from_icon_name(dbusmenu_menuitem_property_get(newitem, APPLICATION_MENUITEM_PROP_ICON), GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request(icon, width
								+ 5 /* ref triangle is 5x9 pixels */
								+ 2 /* padding */,
								height);
	gtk_misc_set_alignment(GTK_MISC(icon), 1.0 /* right aligned */, 0.5);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gmi), icon);
	gtk_widget_show(icon);

	/* Application name in a label */
	GtkWidget * label = gtk_label_new(dbusmenu_menuitem_property_get(newitem, APPLICATION_MENUITEM_PROP_NAME));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, padding);
	gtk_widget_show(label);

	/* Insert the hbox */
	gtk_container_add(GTK_CONTAINER(gmi), hbox);
	gtk_widget_show(hbox);

	/* Attach some of the standard GTK stuff */
	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, gmi, parent);

	/* Make sure we can handle the label changing */
	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(application_prop_change_cb), label);
	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(application_icon_change_cb), icon);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect_after(G_OBJECT (gmi), "draw", G_CALLBACK (application_triangle_draw_cb), newitem);
#else
	g_signal_connect_after(G_OBJECT (gmi), "expose_event", G_CALLBACK (application_triangle_draw_cb), newitem);
#endif
	return TRUE;
}

typedef struct _indicator_item_t indicator_item_t;
struct _indicator_item_t {
	GtkWidget * icon;
	GtkWidget * label;
	GtkWidget * right;
};

/* Whenever we have a property change on a DbusmenuMenuitem
   we need to be responsive to that. */
static void
indicator_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GVariant * value, indicator_item_t * mi_data)
{
	if (!g_strcmp0(prop, INDICATOR_MENUITEM_PROP_LABEL)) {
		/* Set the main label */
		gtk_label_set_text(GTK_LABEL(mi_data->label), g_variant_get_string(value, NULL));
	} else if (!g_strcmp0(prop, INDICATOR_MENUITEM_PROP_RIGHT)) {
		/* Set the right label */
		gtk_label_set_text(GTK_LABEL(mi_data->right), g_variant_get_string(value, NULL));
	} else if (!g_strcmp0(prop, INDICATOR_MENUITEM_PROP_ICON)) {
		/* We don't use the value here, which is probably less efficient, 
		   but it's easier to use the easy function.  And since th value
		   is already cached, shouldn't be a big deal really.  */
		GdkPixbuf * pixbuf = dbusmenu_menuitem_property_get_image(mi, INDICATOR_MENUITEM_PROP_ICON);
		if (pixbuf != NULL) {
			/* If we've got a pixbuf we need to make sure it's of a reasonable
			   size to fit in the menu.  If not, rescale it. */
			GdkPixbuf * resized_pixbuf;
			gint width, height;
			gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
			if (gdk_pixbuf_get_width(pixbuf) > width ||
					gdk_pixbuf_get_height(pixbuf) > height) {
				g_debug("Resizing icon from %dx%d to %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), width, height);
				resized_pixbuf = gdk_pixbuf_scale_simple(pixbuf,
				                                         width,
				                                         height,
				                                         GDK_INTERP_BILINEAR);
			} else {
				g_debug("Happy with icon sized %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
				resized_pixbuf = pixbuf;
			}
	  
			gtk_image_set_from_pixbuf(GTK_IMAGE(mi_data->icon), resized_pixbuf);

			/* The other pixbuf should be free'd by the dbusmenu. */
			if (resized_pixbuf != pixbuf) {
				g_object_unref(resized_pixbuf);
			}

			gtk_widget_show(mi_data->icon);
		} else {
			gtk_widget_hide(mi_data->icon);
		}
	}

	return;
}

/* We have a small little menuitem type that handles all
   of the fun stuff for indicators.  Mostly this is the
   shifting over and putting the icon in with some right
   side text that'll be determined by the service.  */
static gboolean
new_indicator_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client, gpointer user_data)
{
	g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
	g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
	/* Note: not checking parent, it's reasonable for it to be NULL */

	indicator_item_t * mi_data = g_new0(indicator_item_t, 1);

	GtkMenuItem * gmi = GTK_MENU_ITEM(gtk_menu_item_new());

	gint padding = 4;
	gint font_size = RIGHT_LABEL_FONT_SIZE;
	gtk_widget_style_get(GTK_WIDGET(gmi), "horizontal-padding", &padding, NULL);

	GtkWidget * hbox = gtk_hbox_new(FALSE, 0);

	/* Icon, probably someone's face or avatar on an IM */
	mi_data->icon = gtk_image_new();

	/* Set the minimum size, we always want it to take space */
	gint width, height;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
	gtk_widget_set_size_request(mi_data->icon, width, height);

	GdkPixbuf * pixbuf = dbusmenu_menuitem_property_get_image(newitem, INDICATOR_MENUITEM_PROP_ICON);
	if (pixbuf != NULL) {
		/* If we've got a pixbuf we need to make sure it's of a reasonable
		   size to fit in the menu.  If not, rescale it. */
		GdkPixbuf * resized_pixbuf;
		if (gdk_pixbuf_get_width(pixbuf) > width ||
		        gdk_pixbuf_get_height(pixbuf) > height) {
			g_debug("Resizing icon from %dx%d to %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), width, height);
			resized_pixbuf = gdk_pixbuf_scale_simple(pixbuf,
			                                         width,
			                                         height,
			                                         GDK_INTERP_BILINEAR);
		} else {
			g_debug("Happy with icon sized %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
			resized_pixbuf = pixbuf;
		}
  
		gtk_image_set_from_pixbuf(GTK_IMAGE(mi_data->icon), resized_pixbuf);

		/* The other pixbuf should be free'd by the dbusmenu. */
		if (resized_pixbuf != pixbuf) {
			g_object_unref(resized_pixbuf);
		}
	}
	gtk_misc_set_alignment(GTK_MISC(mi_data->icon), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->icon, FALSE, FALSE, padding);

	if (pixbuf != NULL) {
		gtk_widget_show(mi_data->icon);
	}

	/* Label, probably a username, chat room or mailbox name */
	mi_data->label = gtk_label_new(dbusmenu_menuitem_property_get(newitem, INDICATOR_MENUITEM_PROP_LABEL));
	gtk_misc_set_alignment(GTK_MISC(mi_data->label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->label, TRUE, TRUE, padding);
	gtk_widget_show(mi_data->label);

	/* Usually either the time or the count on the individual
	   item. */
	mi_data->right = gtk_label_new(dbusmenu_menuitem_property_get(newitem, INDICATOR_MENUITEM_PROP_RIGHT));
	gtk_size_group_add_widget(indicator_right_group, mi_data->right);
	/* install extra decoration overlay */
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect (G_OBJECT (mi_data->right), "draw",
	                  G_CALLBACK (numbers_draw_cb), NULL);
#else
	g_signal_connect (G_OBJECT (mi_data->right), "expose_event",
	                  G_CALLBACK (numbers_draw_cb), NULL);
#endif

	gtk_misc_set_alignment(GTK_MISC(mi_data->right), 1.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->right, FALSE, FALSE, padding + font_size/2.0);
	gtk_widget_show(mi_data->right);

	gtk_container_add(GTK_CONTAINER(gmi), hbox);
	gtk_widget_show(hbox);

	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, gmi, parent);

	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(indicator_prop_change_cb), mi_data);
	g_object_weak_ref(G_OBJECT(newitem), (GWeakNotify)g_free, mi_data);

	return TRUE;
}

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
	indicator_right_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	DbusmenuGtkMenu * menu = dbusmenu_gtkmenu_new(INDICATOR_MESSAGES_DBUS_NAME, INDICATOR_MESSAGES_DBUS_OBJECT);
	DbusmenuGtkClient * client = dbusmenu_gtkmenu_get_client(menu);

	dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), INDICATOR_MENUITEM_TYPE, new_indicator_item);
	dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client), APPLICATION_MENUITEM_TYPE, new_application_item);

	return GTK_MENU(menu);
}

/* Returns the accessible description of the indicator */
static const gchar *
get_accessible_desc (IndicatorObject * io)
{
	return accessible_desc;
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
