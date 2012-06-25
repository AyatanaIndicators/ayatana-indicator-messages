
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
	messages-service-dbus.c \
	messages-service-dbus.h \
	gen-messages-service.xml.h \
	gen-messages-service.xml.c \
	app-section.c \
	app-section.h \
	dbus-data.h \
	gactionmuxer.c \
	gactionmuxer.h \
	gsettingsstrv.c \
	gsettingsstrv.h

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

CLEANFILES += \
	$(BUILT_SOURCES)

