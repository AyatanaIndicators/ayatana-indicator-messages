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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "status-provider.h"

/* Signals */
enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* GObject Boilerplate */
static void status_provider_class_init (StatusProviderClass *klass);
static void status_provider_init       (StatusProvider *self);

G_DEFINE_TYPE (StatusProvider, status_provider, G_TYPE_OBJECT);

static void
status_provider_class_init (StatusProviderClass *klass)
{
	// GObjectClass *object_class = G_OBJECT_CLASS (klass);

	klass->status_changed = NULL;

	klass->set_status = NULL;
	klass->get_status = NULL;

	/**
		StatusProvider::status-changed:
		@arg0: The #StatusProvider object.
		@arg1: The new status #StatusProviderStatus

		Should be emitted by subclasses everytime that the status
		changes externally to us.
	*/
	signals[STATUS_CHANGED]    = g_signal_new(STATUS_PROVIDER_SIGNAL_STATUS_CHANGED,
	                                          G_TYPE_FROM_CLASS(klass),
	                                          G_SIGNAL_RUN_LAST,
	                                          G_STRUCT_OFFSET(StatusProviderClass, status_changed),
	                                          NULL, NULL,
	                                          g_cclosure_marshal_VOID__UINT,
	                                          G_TYPE_NONE, 1, G_TYPE_UINT);

	return;
}

static void
status_provider_init (StatusProvider *self)
{

	return;
}

void
status_provider_set_status (StatusProvider * sp, StatusProviderStatus status)
{
	g_return_if_fail(IS_STATUS_PROVIDER(sp));

	StatusProviderClass * class = STATUS_PROVIDER_GET_CLASS(sp);
	g_return_if_fail(class != NULL);
	g_return_if_fail(class->set_status != NULL);

	return class->set_status(sp, status);
}

StatusProviderStatus
status_provider_get_status (StatusProvider * sp)
{
	g_return_val_if_fail(IS_STATUS_PROVIDER(sp), STATUS_PROVIDER_STATUS_OFFLINE);

	StatusProviderClass * class = STATUS_PROVIDER_GET_CLASS(sp);
	g_return_val_if_fail(class->get_status != NULL, STATUS_PROVIDER_STATUS_OFFLINE);

	return class->get_status(sp);
}

