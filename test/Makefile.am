
check_LIBRARIES = libgtest.a
check_PROGRAMS = test-gtest

TESTS =

AM_CPPFLAGS = $(GTEST_CPPFLAGS)

nodist_libgtest_a_SOURCES = $(GTEST_SOURCE)/src/gtest-all.cc
libgtest_a_SOURCES = \
	$(GTEST_SOURCE)/src/gtest-all.cc \
	$(GTEST_SOURCE)/src/gtest_main.cc
libgtest_a_CPPFLAGS = \
	$(GTEST_CPPFLAGS) -w
libgtest_a_CXXFLAGS = \
	$(AM_CXXFLAGS)

TESTS += test-gtest

test_gtest_SOURCES = \
	$(top_srcdir)/src/launcher-menu-item.c \
	$(top_srcdir)/src/default-applications.c \
	$(top_srcdir)/src/seen-db.c \
	test-gtest.cpp
test_gtest_CPPFLAGS = \
	$(APPLET_CFLAGS)
test_gtest_LDFLAGS = -pthread
test_gtest_LDADD = \
	$(APPLET_LIBS) \
	libgtest.a
