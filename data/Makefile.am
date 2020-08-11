NULL =

SUBDIRS = icons

xdg_autostartdir = /etc/xdg/autostart
xdg_autostart_DATA = ayatana-indicator-messages.desktop

%.desktop: %.desktop.in
	$(AM_V_GEN) sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

gsettings_SCHEMAS = org.ayatana.indicator.messages.gschema.xml
@GSETTINGS_RULES@

#if defined(HAVE_SYSTEMD)
systemdservice_file = ayatana-indicator-messages.service
$(systemdservice_file): $(systemdservice_file).in
	$(AM_V_GEN) sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

systemddir = $(SYSTEMD_USERDIR)
systemd_DATA = $(systemdservice_file)
#endif

indicatordir = $(INDICATOR_DIR)
dist_indicator_DATA = org.ayatana.indicator.messages

EXTRA_DIST = \
	ayatana-indicator-messages.desktop.in \
	$(gsettings_SCHEMAS) \
	$(systemdservice_file).in \
	$(NULL)

CLEANFILES = \
	$(xdg_autostart_DATA) \
	$(systemdservice_file) \
	$(NULL)
