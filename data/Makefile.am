SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-messages.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

gsettings_SCHEMAS = com.canonical.indicator.messages.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	indicator-messages.service.in

CLEANFILES = indicator-messages.service
