
BUILT_SOURCES = 
EXTRA_DIST =
CLEANFILES = 
DISTCLEANFILES =

libexec_PROGRAMS = indicator-messages-service


######################################
# Building the messages indicator
######################################

messaginglibdir = $(INDICATORDIR)
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = \
	indicator-messages.c \
	indicator-messages-service.c \
	indicator-messages-service.h
	dbus-data.h
libmessaging_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\"
libmessaging_la_LIBADD = $(APPLET_LIBS) -lm
libmessaging_la_LDFLAGS = \
	$(COVERAGE_LDFLAGS) \
	-module -avoid-version

######################################
# Building the messages service
######################################

indicator_messages_service_SOURCES = \
	messages-service.c \
	indicator-messages-service.c \
	indicator-messages-service.h \
	app-section.c \
	app-section.h \
	dbus-data.h \
	gactionmuxer.c \
	gactionmuxer.h \
	gsettingsstrv.c \
	gsettingsstrv.h \
	gmenuutils.c \
	gmenuutils.h

indicator_messages_service_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\"

indicator_messages_service_LDADD = \
	$(APPLET_LIBS)

indicator_messages_service_LDFLAGS = \
	$(COVERAGE_LDFLAGS)

indicator-messages-service.c: $(top_srcdir)/src/messages-service.xml
	$(AM_V_GEN) gdbus-codegen \
	    --interface-prefix com.canonical.indicator.messages. \
	    --generate-c-code indicator-messages-service \
	    --c-namespace IndicatorMessages \
	    $^
indicator-messages-service.h: indicator-messages-service.c

BUILT_SOURCES += \
	indicator-messages-service.c \
	indicator-messages-service.h

EXTRA_DIST += \
	messages-service.xml

CLEANFILES += \
	$(BUILT_SOURCES)

