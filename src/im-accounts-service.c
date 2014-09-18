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

#include <act/act.h>

#include "im-accounts-service.h"

typedef struct _ImAccountsServicePrivate ImAccountsServicePrivate;

struct _ImAccountsServicePrivate {
	ActUserManager * user_manager;
};

#define IM_ACCOUNTS_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsServicePrivate))

static void im_accounts_service_class_init (ImAccountsServiceClass *klass);
static void im_accounts_service_init       (ImAccountsService *self);
static void im_accounts_service_dispose    (GObject *object);
static void im_accounts_service_finalize   (GObject *object);
static void user_changed (ActUserManager * manager, ActUser * user, gpointer user_data);
static void is_loaded (ActUserManager * manager, GParamSpec * pspect, gpointer user_data);

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
	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(self);

	priv->user_manager = act_user_manager_get_default();
	g_signal_connect(priv->user_manager, "user-changed", G_CALLBACK(user_changed), self);
	g_signal_connect(priv->user_manager, "notify::is-loaded", G_CALLBACK(is_loaded), self);
}

static void
im_accounts_service_dispose (GObject *object)
{
	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(object);

	g_clear_object(&priv->user_manager);
	
	G_OBJECT_CLASS (im_accounts_service_parent_class)->dispose (object);
}

static void
im_accounts_service_finalize (GObject *object)
{
	G_OBJECT_CLASS (im_accounts_service_parent_class)->finalize (object);
}

/* Handles a User getting updated */
static void
user_changed (ActUserManager * manager, ActUser * user, gpointer user_data)
{
	if (g_strcmp0(act_user_get_user_name(user), g_get_user_name()) != 0) {
		return;
	}

	g_debug("User Updated");
}

static void
is_loaded (ActUserManager * manager, GParamSpec * pspect, gpointer user_data)
{
	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(user_data);
	ActUser * user = NULL;

	g_debug("Accounts Manager Loaded");

	user = act_user_manager_get_user(priv->user_manager, g_get_user_name());
	if (user != NULL) {
		user_changed(priv->user_manager, user, user_data);
		g_object_unref(user);
	}
}

/* Not the most testable way to do this but, it is a less invasive one, and we'll
   probably restructure this codebase soonish */
/* Gets an account service wrapper reference, so then it can be free'd */
ImAccountsService *
im_accounts_service_ref_default (void)
{
	static ImAccountsService * as = NULL;
	if (as == NULL) {
		as = IM_ACCOUNTS_SERVICE(g_object_new(IM_ACCOUNTS_SERVICE_TYPE, NULL));
		g_object_add_weak_pointer(G_OBJECT(as), (gpointer *)&as);
		return as;
	}

	return g_object_ref(as);
}

void
im_accounts_service_set_draws_attention (ImAccountsService * service, gboolean draws_attention)
{


}

gboolean
im_accounts_service_get_show_on_greeter (ImAccountsService * service)
{

	return FALSE;
}
