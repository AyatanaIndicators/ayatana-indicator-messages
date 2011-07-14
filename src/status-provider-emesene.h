/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 20011 Canonical Ltd.

Authors:
    Stefano Candori <stefano.candori@gmail.com>

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

#ifndef __STATUS_PROVIDER_EMESENE_H__
#define __STATUS_PROVIDER_EMESENE_H__

#include <glib.h>
#include <glib-object.h>

#include "status-provider.h"

G_BEGIN_DECLS

#define STATUS_PROVIDER_EMESENE_TYPE            (status_provider_emesene_get_type ())
#define STATUS_PROVIDER_EMESENE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STATUS_PROVIDER_EMESENE_TYPE, StatusProviderEmesene))
#define STATUS_PROVIDER_EMESENE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STATUS_PROVIDER_EMESENE_TYPE, StatusProviderEmeseneClass))
#define IS_STATUS_PROVIDER_EMESENE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STATUS_PROVIDER_EMESENE_TYPE))
#define IS_STATUS_PROVIDER_EMESENE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STATUS_PROVIDER_EMESENE_TYPE))
#define STATUS_PROVIDER_EMESENE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STATUS_PROVIDER_EMESENE_TYPE, StatusProviderEmeseneClass))


typedef struct _StatusProviderEmeseneClass StatusProviderEmeseneClass;
struct _StatusProviderEmeseneClass {
	StatusProviderClass parent_class;
};

typedef struct _StatusProviderEmesene      StatusProviderEmesene;
struct _StatusProviderEmesene {
	StatusProvider parent;
};

GType status_provider_emesene_get_type (void);
StatusProvider * status_provider_emesene_new (void);

G_END_DECLS

#endif
