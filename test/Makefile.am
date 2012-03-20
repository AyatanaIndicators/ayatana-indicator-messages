
check_LIBRARIES = libgtest.a
check_PROGRAMS = test-gtest

TESTS =

AM_CPPFLAGS = $(GTEST_CPPFLAGS) \
	-I${top_srcdir}/src

nodist_libgtest_a_SOURCES = $(GTEST_SOURCE)/src/gtest-all.cc
libgtest_a_SOURCES = \
	$(GTEST_SOURCE)/src/gtest-all.cc \
	$(GTEST_SOURCE)/src/gtest_main.cc
libgtest_a_CPPFLAGS = \
	$(GTEST_CPPFLAGS) -w
libgtest_a_CXXFLAGS = \
	$(AM_CXXFLAGS)
libgtest_a_CPPFLAGS = \
	$(AM_CPPFLAGS)

TESTS += test-gtest

test_gtest_SOURCES = \
	$(top_srcdir)/src/launcher-menu-item.c \
	$(top_srcdir)/src/default-applications.c \
	$(top_srcdir)/src/seen-db.c \
	test-gtest.cpp
test_gtest_CPPFLAGS = \
	$(APPLET_CFLAGS) \
	$(AM_CPPFLAGS)
test_gtest_LDFLAGS = -pthread
test_gtest_LDADD = \
	$(APPLET_LIBS) \
	libgtest.a


######################################
# Lib containing code under test
######################################

lib_LTLIBRARIES = \
	libindicator-messages-service.la

libindicator_messages_service_la_HEADERS = \
	$(top_srcdir)/src/default-applications.h \
	$(top_srcdir)/src/messages-service-dbus.h \
	$(top_srcdir)/src/gen-messages-service.xml.h \
	$(top_srcdir)/src/im-menu-item.h \
	$(top_srcdir)/src/app-menu-item.h \
	$(top_srcdir)/src/launcher-menu-item.h \
	$(top_srcdir)/src/seen-db.h \
	$(top_srcdir)/src/dirs.h \
	$(top_srcdir)/src/dbus-data.h \
	$(top_srcdir)/src/status-items.h

libindicator_messages_service_la_SOURCES = \
	$(top_srcdir)/src/default-applications.c \
	$(top_srcdir)/src/messages-service.c \
	$(top_srcdir)/src/messages-service-dbus.c \
	$(top_srcdir)/src/gen-messages-service.xml.c \
	$(top_srcdir)/src/im-menu-item.c \
	$(top_srcdir)/src/app-menu-item.c \
	$(top_srcdir)/src/launcher-menu-item.c \
	$(top_srcdir)/src/seen-db.c \
	$(top_srcdir)/src/status-items.c

libindicator_messages_service_ladir = \
	$(includedir)/libindicator-messages-service/

libindicator_messages_service_la_CFLAGS = \
	$(APPLET_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall \
	-Wl,-Bsymbolic-functions \
	-Wl,-z,defs \
	-Wl,--as-needed \
	-Werror \
	-DG_LOG_DOMAIN=\"Indicator-Messages\" \
	-DSTATUS_PROVIDER_DIR=\"$(STATUS_PROVIDER_DIR)\"

libindicator_messages_service_la_LIBADD = \
	$(APPLET_LIBS) \
	libindicator-messages-status-provider.la

libindicator_messages_service_la_LDFLAGS = \
	$(COVERAGE_LDFLAGS)

