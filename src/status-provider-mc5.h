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

#ifndef __STATUS_PROVIDER_MC5_H__
#define __STATUS_PROVIDER_MC5_H__

#include <glib.h>
#include <glib-object.h>

#include "status-provider.h"

G_BEGIN_DECLS

#define STATUS_PROVIDER_MC5_TYPE            (status_provider_mc5_get_type ())
#define STATUS_PROVIDER_MC5(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STATUS_PROVIDER_MC5_TYPE, StatusProviderMC5))
#define STATUS_PROVIDER_MC5_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STATUS_PROVIDER_MC5_TYPE, StatusProviderMC5Class))
#define IS_STATUS_PROVIDER_MC5(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STATUS_PROVIDER_MC5_TYPE))
#define IS_STATUS_PROVIDER_MC5_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STATUS_PROVIDER_MC5_TYPE))
#define STATUS_PROVIDER_MC5_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STATUS_PROVIDER_MC5_TYPE, StatusProviderMC5Class))


typedef struct _StatusProviderMC5Class StatusProviderMC5Class;
struct _StatusProviderMC5Class {
	StatusProviderClass parent_class;
};

typedef struct _StatusProviderMC5      StatusProviderMC5;
struct _StatusProviderMC5 {
	StatusProvider parent;
};

GType status_provider_mc5_get_type (void);
StatusProvider * status_provider_mc5_new (void);

G_END_DECLS

#endif
