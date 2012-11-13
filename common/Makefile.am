
noinst_LTLIBRARIES = libmessaging-common.la

indicator-messages-service.c: com.canonical.indicator.messages.service.xml
	$(AM_V_GEN) gdbus-codegen \
	    --interface-prefix com.canonical.indicator.messages. \
	    --generate-c-code indicator-messages-service \
	    --c-namespace IndicatorMessages \
	    $^
indicator-messages-service.h: indicator-messages-service.c

BUILT_SOURCES = \
	indicator-messages-service.c \
	indicator-messages-service.h

libmessaging_common_la_SOURCES = \
	$(BUILT_SOURCES)

libmessaging_common_la_CFLAGS = $(GIO_CFLAGS)
libmessaging_common_la_LIBADD = $(GIO_LIBS)

CLEANFILES = $(BUILT_SOURCES)
