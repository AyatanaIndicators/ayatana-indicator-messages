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

#ifndef __IM_ACCOUNTS_SERVICE_H__
#define __IM_ACCOUNTS_SERVICE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define IM_ACCOUNTS_SERVICE_TYPE            (im_accounts_service_get_type ())
#define IM_ACCOUNTS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsService))
#define IM_ACCOUNTS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsServiceClass))
#define IM_IS_ACCOUNTS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_ACCOUNTS_SERVICE_TYPE))
#define IM_IS_ACCOUNTS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IM_ACCOUNTS_SERVICE_TYPE))
#define IM_ACCOUNTS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsServiceClass))

typedef struct _ImAccountsService      ImAccountsService;
typedef struct _ImAccountsServiceClass ImAccountsServiceClass;

struct _ImAccountsServiceClass {
	GObjectClass parent_class;
};

struct _ImAccountsService {
	GObject parent;
};

GType im_accounts_service_get_type (void);
ImAccountsService * im_accounts_service_ref_default (void);
void im_accounts_service_set_draws_attention (ImAccountsService * service, gboolean draws_attention);
gboolean im_accounts_service_get_show_on_greeter (ImAccountsService * service);

G_END_DECLS

#endif
