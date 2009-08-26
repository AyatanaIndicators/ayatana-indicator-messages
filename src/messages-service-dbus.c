#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "messages-service-dbus.h"

typedef struct _MessageServiceDbusPrivate MessageServiceDbusPrivate;

struct _MessageServiceDbusPrivate
{
	guint temp;
};

#define MESSAGE_SERVICE_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbusPrivate))

static void message_service_dbus_class_init (MessageServiceDbusClass *klass);
static void message_service_dbus_init       (MessageServiceDbus *self);
static void message_service_dbus_dispose    (GObject *object);
static void message_service_dbus_finalize   (GObject *object);

static void _messages_service_server_watch  (void);
static void _messages_service_server_attention_requested (void);
static void _messages_service_server_icon_shown (void);

#include "messages-service-server.h"

G_DEFINE_TYPE (MessageServiceDbus, message_service_dbus, G_TYPE_OBJECT);

static void
message_service_dbus_class_init (MessageServiceDbusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MessageServiceDbusPrivate));

	object_class->dispose = message_service_dbus_dispose;
	object_class->finalize = message_service_dbus_finalize;

	return;
}

static void
message_service_dbus_init (MessageServiceDbus *self)
{
	return;
}

static void
message_service_dbus_dispose (GObject *object)
{


	G_OBJECT_CLASS (message_service_dbus_parent_class)->dispose (object);
	return;
}

static void
message_service_dbus_finalize (GObject *object)
{


	G_OBJECT_CLASS (message_service_dbus_parent_class)->finalize (object);
	return;
}

MessageServiceDbus *
message_service_dbus_new (void)
{
	return MESSAGE_SERVICE_DBUS(g_object_new(MESSAGE_SERVICE_DBUS_TYPE, NULL));
}

static void
_messages_service_server_watch  (void)
{

}

static void
_messages_service_server_attention_requested (void)
{

}

static void
_messages_service_server_icon_shown (void)
{

}

