
lib_LTLIBRARIES = libmessaging-menu.la

libmessaging_menu_ladir = $(includedir)/messaging-menu

libmessaging_menu_la_SOURCES = \
	messaging-menu.c \
	$(BUILT_SOURCES)

libmessaging_menu_la_HEADERS = \
	messaging-menu.h

libmessaging_menu_la_LIBADD = $(GIO_LIBS)

libmessaging_menu_la_CFLAGS = \
	$(GIO_CFLAGS) \
	-Wall

BUILT_SOURCES = \
	indicator-messages-service.c \
	indicator-messages-service.h

CLEANFILES = $(BUILT_SOURCES)

indicator-messages-service.c: $(top_srcdir)/src/messages-service.xml
	$(AM_V_GEN) gdbus-codegen \
	    --interface-prefix com.canonical.indicator.messages. \
	    --generate-c-code indicator-messages-service \
	    --c-namespace IndicatorMessages \
	    $^
indicator-messages-service.h: indicator-messages-service.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = messaging-menu.pc

