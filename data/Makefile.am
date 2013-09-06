SUBDIRS = icons

dbus_servicesdir = $(datadir)/dbus-1/services
dbus_services_DATA = indicator-messages.service

%.service: %.service.in
	sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

gsettings_SCHEMAS = com.canonical.indicator.messages.gschema.xml
@GSETTINGS_RULES@

indicatordir = $(INDICATOR_DIR)
dist_indicator_DATA = com.canonical.indicator.messages

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	indicator-messages.service.in

CLEANFILES = indicator-messages.service
