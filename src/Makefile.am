
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
	gen-messages-service.xml.h \
	gen-messages-service.xml.c \
	dbus-data.h
libmessaging_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\"
libmessaging_la_LIBADD = $(APPLET_LIBS) -lm
libmessaging_la_LDFLAGS = -module -avoid-version

######################################
# Building the messages service
######################################

indicator_messages_service_SOURCES = \
	default-applications.h \
	default-applications.c \
	messages-service.c \
	messages-service-dbus.c \
	messages-service-dbus.h \
	gen-messages-service.xml.h \
	gen-messages-service.xml.c \
	im-menu-item.c \
	im-menu-item.h \
	app-menu-item.c \
	app-menu-item.h \
	launcher-menu-item.c \
	launcher-menu-item.h \
	seen-db.c \
	seen-db.h \
	dirs.h \
	dbus-data.h \
	\
	status-items.c \
	status-items.h

indicator_messages_service_CFLAGS = \
	$(APPLET_CFLAGS) \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\" \
	-DSTATUS_PROVIDER_DIR=\"$(STATUS_PROVIDER_DIR)\"

indicator_messages_service_LDADD = \
	$(APPLET_LIBS) \
	libindicator-messages-status-provider.la

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<))));" > $@

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	echo "const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<)))) = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@

BUILT_SOURCES += \
	gen-messages-service.xml.h \
	gen-messages-service.xml.c

EXTRA_DIST += \
	messages-service.xml

######################################
# Status Provider Library
######################################

STATUS_PROVIDER_API_VERSION = 1
STATUS_PROVIDER_DIR = $(libexecdir)/status-providers/$(STATUS_PROVIDER_API_VERSION)
statusprovidersdir = $(STATUS_PROVIDER_DIR)
statusproviders_LTLIBRARIES = 

EXTRA_DIST += \
	indicator-messages-status-provider-0.5.pc.in.in
CLEANFILES += \
	indicator-messages-status-provider-0.5.pc

pkgconfig_DATA = indicator-messages-status-provider-0.5.pc
pkgconfigdir = $(libdir)/pkgconfig

%.pc: %.pc.in
	sed \
		-e "s|\@status_provider_dir\@|$(STATUS_PROVIDER_DIR)|" \
		-e "s|\@status_provider_api_version\@|$(STATUS_PROVIDER_API_VERSION)|" \
		$< > $@

lib_LTLIBRARIES = \
	libindicator-messages-status-provider.la

libindicator_messages_status_provider_la_HEADERS = \
	status-provider.h

libindicator_messages_status_provider_la_SOURCES = \
	$(libindicator_messages_status_provider_HEADERS) \
	status-provider.c

libindicator_messages_status_provider_ladir = \
	$(includedir)/libindicator-messages-status-provider-$(STATUS_PROVIDER_API_VERSION)/

libindicator_messages_status_provider_la_LDFLAGS = \
	-version-info $(STATUS_PROVIDER_API_VERSION):0:0 \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

libindicator_messages_status_provider_la_LIBADD = \
	$(APPLET_LIBS)

libindicator_messages_status_provider_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	-Wall -Werror

######################################
# Status provider: Pidgin
######################################

statusproviders_LTLIBRARIES += libpidgin.la
libpidgin_la_SOURCES = \
	status-provider-pidgin.h \
	status-provider-pidgin.c \
	status-provider-pidgin-marshal.h \
	status-provider-pidgin-marshal.c
libpidgin_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(STATUS_PROVIDER_PIDGIN_CFLAGS) \
	-Wall -Werror \
	-DG_LOG_DOMAIN=\"Status-Provider-Pidgin\"
libpidgin_la_LIBADD = \
	libindicator-messages-status-provider.la \
	$(APPLET_LIBS) \
	$(STATUS_PROVIDER_PIDGIN_LIBS)
libpidgin_la_LDFLAGS = -module -avoid-version

status-provider-pidgin-marshal.h: $(srcdir)/status-provider-pidgin.list
	glib-genmarshal --header \
		--prefix=_status_provider_pidgin_marshal $(srcdir)/status-provider-pidgin.list \
		> status-provider-pidgin-marshal.h

status-provider-pidgin-marshal.c: $(srcdir)/status-provider-pidgin.list
	glib-genmarshal --body \
		--prefix=_status_provider_pidgin_marshal $(srcdir)/status-provider-pidgin.list \
		> status-provider-pidgin-marshal.c

BUILT_SOURCES += \
	status-provider-pidgin-marshal.h \
	status-provider-pidgin-marshal.c 

EXTRA_DIST += \
	status-provider-pidgin.list

######################################
# Status provider: Mission Control 4
######################################

statusproviders_LTLIBRARIES += libtelepathy.la
libtelepathy_la_SOURCES = \
	status-provider-telepathy.h \
	status-provider-telepathy.c \
	status-provider-telepathy-marshal.h \
	status-provider-telepathy-marshal.c
libtelepathy_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(STATUS_PROVIDER_TELEPATHY_CFLAGS) \
	-Wall -Werror \
	-DG_LOG_DOMAIN=\"Status-Provider-Telepathy\"
libtelepathy_la_LIBADD = \
	libindicator-messages-status-provider.la \
	$(APPLET_LIBS) \
	$(STATUS_PROVIDER_TELEPATHY_LIBS)
libtelepathy_la_LDFLAGS = -module -avoid-version

status-provider-telepathy-marshal.h: $(srcdir)/status-provider-telepathy.list
	glib-genmarshal --header \
		--prefix=_status_provider_telepathy_marshal $(srcdir)/status-provider-telepathy.list \
		> status-provider-telepathy-marshal.h

status-provider-telepathy-marshal.c: $(srcdir)/status-provider-telepathy.list
	glib-genmarshal --body \
		--prefix=_status_provider_telepathy_marshal $(srcdir)/status-provider-telepathy.list \
		> status-provider-telepathy-marshal.c

BUILT_SOURCES += \
	status-provider-telepathy-marshal.h \
	status-provider-telepathy-marshal.c

EXTRA_DIST += \
	status-provider-telepathy.list

######################################
# Status provider: Mission Control 5
######################################

statusproviders_LTLIBRARIES += libmc5.la
libmc5_la_SOURCES = \
	status-provider-mc5.h \
	status-provider-mc5.c \
	status-provider-mc5-marshal.h \
	status-provider-mc5-marshal.c
libmc5_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(STATUS_PROVIDER_MC5_CFLAGS) \
	-Wall -Werror \
	-DG_LOG_DOMAIN=\"Status-Provider-MC5\"
libmc5_la_LIBADD = \
	libindicator-messages-status-provider.la \
	$(APPLET_LIBS) \
	$(STATUS_PROVIDER_MC5_LIBS)
libmc5_la_LDFLAGS = -module -avoid-version

status-provider-mc5-marshal.h: $(srcdir)/status-provider-mc5.list
	glib-genmarshal --header \
		--prefix=_status_provider_mc5_marshal $(srcdir)/status-provider-mc5.list \
		> status-provider-mc5-marshal.h

status-provider-mc5-marshal.c: $(srcdir)/status-provider-mc5.list
	glib-genmarshal --body \
		--prefix=_status_provider_mc5_marshal $(srcdir)/status-provider-mc5.list \
		> status-provider-mc5-marshal.c

BUILT_SOURCES += \
	status-provider-mc5-marshal.h \
	status-provider-mc5-marshal.c

EXTRA_DIST += \
	status-provider-mc5.list

######################################
# Status provider: Emesene
######################################

statusproviders_LTLIBRARIES += libemesene.la
libemesene_la_SOURCES = \
	status-provider-emesene.h \
	status-provider-emesene.c
libemesene_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(STATUS_PROVIDER_EMESENE_CFLAGS) \
	-Wall -Werror \
	-DG_LOG_DOMAIN=\"Status-Provider-Emesene\"
libemesene_la_LIBADD = \
	libindicator-messages-status-provider.la \
	$(APPLET_LIBS) \
	$(STATUS_PROVIDER_EMESENE_LIBS)
libemesene_la_LDFLAGS = -module -avoid-version

######################################
# Extras
######################################

CLEANFILES += \
	$(BUILT_SOURCES)

