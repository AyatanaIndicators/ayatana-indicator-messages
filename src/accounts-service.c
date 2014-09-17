#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "accounts-service.h"

typedef struct _AccountsServicePrivate AccountsServicePrivate;

struct _AccountsServicePrivate {
	int dummy;
};

#define ACCOUNTS_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), ACCOUNTS_SERVICE_TYPE, AccountsServicePrivate))

static void accounts_service_class_init (AccountsServiceClass *klass);
static void accounts_service_init       (AccountsService *self);
static void accounts_service_dispose    (GObject *object);
static void accounts_service_finalize   (GObject *object);

G_DEFINE_TYPE (AccountsService, accounts_service, G_TYPE_OBJECT);

static void
accounts_service_class_init (AccountsServiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AccountsServicePrivate));

	object_class->dispose = accounts_service_dispose;
	object_class->finalize = accounts_service_finalize;
}

static void
accounts_service_init (AccountsService *self)
{
}

static void
accounts_service_dispose (GObject *object)
{
	G_OBJECT_CLASS (accounts_service_parent_class)->dispose (object);
}

static void
accounts_service_finalize (GObject *object)
{
	G_OBJECT_CLASS (accounts_service_parent_class)->finalize (object);
}

AccountsService *
accounts_service_ref_default (void)
{

	return NULL;
}
