
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
	dbus-data.h
indicator_messages_service_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
indicator_messages_service_LDADD = $(APPLET_LIBS)

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<))));" > $@

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	echo "const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<)))) = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@

BUILT_SOURCES = \
	gen-messages-service.xml.h \
	gen-messages-service.xml.c

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	messages-service.xml

