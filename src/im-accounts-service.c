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
	GDBusProxy * touch_settings;
	GCancellable * cancel;
};

#define IM_ACCOUNTS_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), IM_ACCOUNTS_SERVICE_TYPE, ImAccountsServicePrivate))

static void im_accounts_service_class_init (ImAccountsServiceClass *klass);
static void im_accounts_service_init       (ImAccountsService *self);
static void im_accounts_service_dispose    (GObject *object);
static void im_accounts_service_finalize   (GObject *object);
static void user_changed (ActUserManager * manager, ActUser * user, gpointer user_data);
static void on_user_manager_loaded (ActUserManager * manager, GParamSpec * pspect, gpointer user_data);
static void security_privacy_ready (GObject * obj, GAsyncResult * res, gpointer user_data);

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

	priv->cancel = g_cancellable_new();

	priv->user_manager = act_user_manager_get_default();
	g_signal_connect(priv->user_manager, "user-changed", G_CALLBACK(user_changed), self);
	g_signal_connect(priv->user_manager, "notify::is-loaded", G_CALLBACK(on_user_manager_loaded), self);

	gboolean isLoaded = FALSE;
	g_object_get(G_OBJECT(priv->user_manager), "is-loaded", &isLoaded, NULL);
	if (isLoaded) {
		on_user_manager_loaded(priv->user_manager, NULL, NULL);
	}
}

static void
im_accounts_service_dispose (GObject *object)
{
	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(object);

	if (priv->cancel != NULL) {
		g_cancellable_cancel(priv->cancel);
		g_clear_object(&priv->cancel);
	}

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

	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(user_data);
	g_debug("User Updated");

	/* Clear old proxies */
	g_clear_object(&priv->touch_settings);

	/* Start getting a new proxy */
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		"org.freedesktop.Accounts",
		act_user_get_object_path(user),
		"com.ubuntu.touch.AccountsService.SecurityPrivacy",
		priv->cancel,
		security_privacy_ready,
		user_data);
}

/* Respond to the async of setting up the proxy. Mostly we get it or we error. */
static void
security_privacy_ready (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (error != NULL) {
		g_warning("Unable to get a proxy on accounts service for touch settings: %s", error->message);
		g_error_free(error);
		return;
	}

	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(user_data);
	/* Ensure we didn't get a proxy while we weren't looking */
	g_clear_object(&priv->touch_settings);
	priv->touch_settings = proxy;
}

/* When the user manager is loaded see if we have a user already loaded
   along with. */
static void
on_user_manager_loaded (ActUserManager * manager, GParamSpec * pspect, gpointer user_data)
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

/* The draws attention setting is very legacy right now, we've patched and not changed
   things much. We're gonna do better in the future, this function abstracts out the ugly */
void
im_accounts_service_set_draws_attention (ImAccountsService * service, gboolean draws_attention)
{
	g_return_if_fail(IM_IS_ACCOUNTS_SERVICE(service));
	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(service);

	if (priv->touch_settings == NULL) {
		return;
	}

	g_dbus_connection_call(g_dbus_proxy_get_connection(priv->touch_settings),
		g_dbus_proxy_get_name(priv->touch_settings),
		g_dbus_proxy_get_object_path(priv->touch_settings),
		"org.freedesktop.Accounts.User",
		"SetXHasMessages",
		g_variant_new("(b)", draws_attention),
		NULL, /* reply */
		G_DBUS_CALL_FLAGS_NONE,
		-1, /* timeout */
		priv->cancel, /* cancellable */
		NULL, NULL); /* cb */
}

/* Looks at the property that is set by settings. We default to off in any case
   that we can or we don't know what the state is. */
gboolean
im_accounts_service_get_show_on_greeter (ImAccountsService * service)
{
	g_return_val_if_fail(IM_IS_ACCOUNTS_SERVICE(service), FALSE);

	ImAccountsServicePrivate * priv = IM_ACCOUNTS_SERVICE_GET_PRIVATE(service);

	if (priv->touch_settings == NULL) {
		return FALSE;
	}

	GVariant * val = g_dbus_proxy_get_cached_property(priv->touch_settings, "MessagesWelcomeScreen");
	if (val == NULL) {
		return FALSE;
	}

	gboolean retval = g_variant_get_boolean(val);
	g_variant_unref(val);
	return retval;
}
