
bin_PROGRAMS = indicator-messages-service

######################################
# Building the messages indicator
######################################

messaginglibdir = $(INDICATORDIR)
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = \
	indicator-messages.c\
	dbus-data.h
libmessaging_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
libmessaging_la_LIBADD = $(APPLET_LIBS)
libmessaging_la_LDFLAGS = -module -avoid-version

######################################
# Building the messages service
######################################

indicator_messages_service_SOURCES = \
	messages-service.c \
	im-menu-item.c \
	im-menu-item.h \
	app-menu-item.c \
	app-menu-item.h \
	launcher-menu-item.c \
	launcher-menu-item.h \
	dirs.h \
	dbus-data.h
indicator_messages_service_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
indicator_messages_service_LDADD = $(APPLET_LIBS)
