SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-messages.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = indicator-messages.service.in

CLEANFILES = indicator-messages.service
