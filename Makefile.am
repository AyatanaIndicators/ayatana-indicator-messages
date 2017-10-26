SUBDIRS = \
	common \
	src \
	libmessaging-menu \
	data \
	po

if ENABLE_GTK_DOC
SUBDIRS += doc
endif

if BUILD_TESTS
SUBDIRS += \
	tests

# build src first
tests: src libayatana-messaging-menu

endif

include $(top_srcdir)/Makefile.am.coverage
