SUBDIRS = icons

upstart_jobsdir = $(datadir)/upstart/sessions/
upstart_jobs_DATA = indicator-messages.conf

%.conf: %.conf.in
	$(AM_V_GEN) sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

gsettings_SCHEMAS = com.canonical.indicator.messages.gschema.xml
@GSETTINGS_RULES@

indicatordir = $(INDICATOR_DIR)
dist_indicator_DATA = com.canonical.indicator.messages

EXTRA_DIST = \
	indicator-messages.conf.in \
	$(gsettings_SCHEMAS)

CLEANFILES = \
	$(upstart_jobs_DATA)
