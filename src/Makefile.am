
libexec_PROGRAMS = indicator-messages-service

######################################
# Building the messages indicator
######################################

messaginglibdir = $(INDICATORDIR)
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = \
	indicator-messages.c \
	messages-service-client.h \
	dbus-data.h
libmessaging_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
libmessaging_la_LIBADD = $(APPLET_LIBS)
libmessaging_la_LDFLAGS = -module -avoid-version

######################################
# Building the messages service
######################################

indicator_messages_service_SOURCES = \
	default-applications.h \
	default-applications.c \
	messages-service.c \
	messages-service-server.h \
	messages-service-dbus.c \
	messages-service-dbus.h \
	im-menu-item.c \
	im-menu-item.h \
	app-menu-item.c \
	app-menu-item.h \
	launcher-menu-item.c \
	launcher-menu-item.h \
	seen-db.c \
	seen-db.h \
	dirs.h \
	dbus-data.h
indicator_messages_service_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
indicator_messages_service_LDADD = $(APPLET_LIBS)

messages-service-client.h: $(srcdir)/messages-service.xml
	dbus-binding-tool \
		--prefix=_messages_service_client \
		--mode=glib-client \
		--output=messages-service-client.h \
		$(srcdir)/messages-service.xml

messages-service-server.h: $(srcdir)/messages-service.xml
	dbus-binding-tool \
		--prefix=_messages_service_server \
		--mode=glib-server \
		--output=messages-service-server.h \
		$(srcdir)/messages-service.xml

BUILT_SOURCES = \
	messages-service-client.h \
	messages-service-server.h

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	messages-service.xml

