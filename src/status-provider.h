/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

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

#ifndef __STATUS_PROVIDER_H__
#define __STATUS_PROVIDER_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define STATUS_PROVIDER_EXPORT_TYPE(x)  GType status_provider_export_type (void) { return (x); }
#define STATUS_PROVIDER_EXPORT_S             "status_provider_export_type"

#define STATUS_PROVIDER_TYPE            (status_provider_get_type ())
#define STATUS_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STATUS_PROVIDER_TYPE, StatusProvider))
#define STATUS_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STATUS_PROVIDER_TYPE, StatusProviderClass))
#define IS_STATUS_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STATUS_PROVIDER_TYPE))
#define IS_STATUS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STATUS_PROVIDER_TYPE))
#define STATUS_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STATUS_PROVIDER_TYPE, StatusProviderClass))

typedef enum
{
  STATUS_PROVIDER_STATUS_ONLINE,
  STATUS_PROVIDER_STATUS_AWAY,
  STATUS_PROVIDER_STATUS_DND,
  STATUS_PROVIDER_STATUS_INVISIBLE,
  STATUS_PROVIDER_STATUS_OFFLINE,
  STATUS_PROVIDER_STATUS_DISCONNECTED,
  /* Leave as last */
  STATUS_PROVIDER_STATUS_LAST
}
StatusProviderStatus;

#define STATUS_PROVIDER_SIGNAL_STATUS_CHANGED     "status-changed"
#define STATUS_PROVIDER_SIGNAL_STATUS_CHANGED_ID  (g_signal_lookup(STATUS_PROVIDER_SIGNAL_STATUS_CHANGED, STATUS_PROVIDER_TYPE))

typedef struct _StatusProvider      StatusProvider;
struct _StatusProvider {
	GObject parent;
};

typedef struct _StatusProviderClass StatusProviderClass;
struct _StatusProviderClass {
	GObjectClass parent_class;

	/* Signals */
	void (*status_changed) (StatusProviderStatus newstatus);

	/* Virtual Functions */
	void  (*set_status) (StatusProvider * sp, StatusProviderStatus newstatus);
	StatusProviderStatus (*get_status) (StatusProvider * sp);
};

GType status_provider_get_type (void);

void status_provider_set_status (StatusProvider * sp, StatusProviderStatus status);
StatusProviderStatus status_provider_get_status (StatusProvider * sp);

G_END_DECLS

#endif
