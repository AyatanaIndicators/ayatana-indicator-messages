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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "im-accounts-service.h"

typedef struct _ImAccountsServicePrivate ImAccountsServicePrivate;

struct _ImAccountsServicePrivate {
	int dummy;
};

#define IM_ACCOUNTS_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsServicePrivate))

static void im_accounts_service_class_init (ImAccountsServiceClass *klass);
static void im_accounts_service_init       (ImAccountsService *self);
static void im_accounts_service_dispose    (GObject *object);
static void im_accounts_service_finalize   (GObject *object);

G_DEFINE_TYPE (ImAccountsService, im_accounts_service, G_TYPE_OBJECT);

static void
im_accounts_service_class_init (ImAccountsServiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ImAccountsServicePrivate));

	object_class->dispose = im_accounts_service_dispose;
	object_class->finalize = im_accounts_service_finalize;
}

static void
im_accounts_service_init (ImAccountsService *self)
{
}

static void
im_accounts_service_dispose (GObject *object)
{
	G_OBJECT_CLASS (im_accounts_service_parent_class)->dispose (object);
}

static void
im_accounts_service_finalize (GObject *object)
{
	G_OBJECT_CLASS (im_accounts_service_parent_class)->finalize (object);
}

ImAccountsService *
im_accounts_service_ref_default (void)
{

	return NULL;
}
