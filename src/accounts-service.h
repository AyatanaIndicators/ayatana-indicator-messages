/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Ted Gould <ted@canonical.com>
 */

#ifndef __ACCOUNTS_SERVICE_H__
#define __ACCOUNTS_SERVICE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define ACCOUNTS_SERVICE_TYPE            (accounts_service_get_type ())
#define ACCOUNTS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ACCOUNTS_SERVICE_TYPE, AccountsService))
#define ACCOUNTS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ACCOUNTS_SERVICE_TYPE, AccountsServiceClass))
#define IS_ACCOUNTS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ACCOUNTS_SERVICE_TYPE))
#define IS_ACCOUNTS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ACCOUNTS_SERVICE_TYPE))
#define ACCOUNTS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ACCOUNTS_SERVICE_TYPE, AccountsServiceClass))

typedef struct _AccountsService      AccountsService;
typedef struct _AccountsServiceClass AccountsServiceClass;

struct _AccountsServiceClass {
	GObjectClass parent_class;
};

struct _AccountsService {
	GObject parent;
};

GType accounts_service_get_type (void);
AccountsService * accounts_service_ref_default (void);

G_END_DECLS

#endif
