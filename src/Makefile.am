
EXTRA_DIST =

libexec_PROGRAMS = indicator-messages-service


######################################
# Building the messages indicator
######################################

messaginglibdir = $(INDICATORDIR)
messaginglib_LTLIBRARIES = libmessaging.la
libmessaging_la_SOURCES = \
	indicator-messages.c \
	ido-menu-item.c \
	ido-menu-item.h \
	im-app-menu-item.c \
	im-app-menu-item.h \
	im-source-menu-item.c \
	im-source-menu-item.h \
	ido-detail-label.c \
	ido-detail-label.h \
	dbus-data.h
libmessaging_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_builddir)/common \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\"
libmessaging_la_LIBADD = \
	$(top_builddir)/common/libmessaging-common.la \
	$(APPLET_LIBS) -lm
libmessaging_la_LDFLAGS = \
	$(COVERAGE_LDFLAGS) \
	-module -avoid-version

######################################
# Building the messages service
######################################

indicator_messages_service_SOURCES = \
	messages-service.c \
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
	-I$(top_builddir)/common \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\"

indicator_messages_service_LDADD = \
	$(top_builddir)/common/libmessaging-common.la \
	$(APPLET_LIBS)

indicator_messages_service_LDFLAGS = \
	$(COVERAGE_LDFLAGS)

EXTRA_DIST += \
	messages-service.xml
