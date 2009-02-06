
messaginglibdir = $(libdir)/indicators/1
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = indicator-messages.c im-menu-item.c im-menu-item.h app-menu-item.c app-menu-item.h
libmessaging_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed
libmessaging_la_LIBADD = $(APPLET_LIBS)
