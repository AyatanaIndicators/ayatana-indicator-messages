
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
	test-gtest.cpp
test_gtest_CPPFLAGS = \
	$(APPLET_CFLAGS) \
	-I$(top_srcdir)/src
test_gtest_LDFLAGS = -pthread
test_gtest_LDADD = \
	$(APPLET_LIBS) \
	libgtest.a
