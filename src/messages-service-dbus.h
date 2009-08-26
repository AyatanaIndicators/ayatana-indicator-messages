#ifndef __MESSAGE_SERVICE_DBUS_H__
#define __MESSAGE_SERVICE_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MESSAGE_SERVICE_DBUS_TYPE            (message_service_dbus_get_type ())
#define MESSAGE_SERVICE_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbus))
#define MESSAGE_SERVICE_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbusClass))
#define IS_MESSAGE_SERVICE_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MESSAGE_SERVICE_DBUS_TYPE))
#define IS_MESSAGE_SERVICE_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MESSAGE_SERVICE_DBUS_TYPE))
#define MESSAGE_SERVICE_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MESSAGE_SERVICE_DBUS_TYPE, MessageServiceDbusClass))

#define MESSAGE_SERVICE_DBUS_SIGNAL_ATTENTION_CHANGED  "attention-changed"
#define MESSAGE_SERVICE_DBUS_SIGNAL_ICON_CHANGED       "icon-changed"

typedef struct _MessageServiceDbus      MessageServiceDbus;
typedef struct _MessageServiceDbusClass MessageServiceDbusClass;

struct _MessageServiceDbusClass {
	GObjectClass parent_class;

	void (*attention_changed) (gboolean dot);
	void (*icon_changed) (gboolean hidden);
};

struct _MessageServiceDbus {
	GObject parent;
};

GType message_service_dbus_get_type (void);
MessageServiceDbus * message_service_dbus_new (void);
void message_service_dbus_set_attention (gboolean attention);
void message_service_dbus_set_icon (gboolean hidden);

G_END_DECLS

#endif
