
lib_LTLIBRARIES = libmessaging-menu.la

libmessaging_menu_ladir = $(includedir)/messaging-menu

libmessaging_menu_la_SOURCES = \
	messaging-menu.c \
	gtupleaction.c \
	gtupleaction.h \
	$(BUILT_SOURCES)

libmessaging_menu_la_HEADERS = \
	messaging-menu.h

libmessaging_menu_la_LIBADD = $(GIO_LIBS)

libmessaging_menu_la_CFLAGS = \
	$(GIO_CFLAGS) \
	-Wall

libmessaging_menu_la_LDFLAGS = -export-symbols-regex "^messaging_menu_.*"

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


-include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

MessagingMenu-1.0.gir: libmessaging-menu.la
MessagingMenu_1_0_gir_NAMESPACE = MessagingMenu
MessagingMenu_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
MessagingMenu_1_0_gir_CFLAGS = $(INCLUDES) $(GIO_CFLAGS)
MessagingMenu_1_0_gir_SCANNERFLAGS = --c-include="messaging-menu.h"
MessagingMenu_1_0_gir_LIBS = libmessaging-menu.la
MessagingMenu_1_0_gir_FILES = messaging-menu.c messaging-menu.h
MessagingMenu_1_0_gir_EXPORT_PACKAGES = messaging-menu
INTROSPECTION_GIRS += MessagingMenu-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES +=$(gir_DATA) $(typelib_DATA)
endif
