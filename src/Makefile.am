
bin_PROGRAMS = indicator-messages-service

messaginglibdir = $(INDICATORDIR)
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = indicator-messages.c im-menu-item.c im-menu-item.h app-menu-item.c app-menu-item.h
libmessaging_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed
libmessaging_la_LIBADD = $(APPLET_LIBS)
libmessaging_la_LDFLAGS = -module -avoid-version

indicator_messages_service_SOURCES = indicator-service.c im-menu-item.c im-menu-item.h app-menu-item.c app-menu-item.h
indicator_messages_service_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed
indicator_messages_service_LDADD = $(APPLET_LIBS)
