
check_LIBRARIES = libgtest.a

AM_CPPFLAGS = $(GTEST_CPPFLAGS)

nodist_libgtest_a_SOURCES = $(GTEST_SOURCE)/src/gtest-all.cc
libgtest_a_CPPFLAGS = $(GTEST_CPPFLAGS) -w
libgtest_a_CXXFLAGS = $(AM_CXXFLAGS)

libgtest_a_SOURCES = \
	$(GTEST_SOURCE)/src/gtest-all.cc \
	$(GTEST_SOURCE)/src/gtest-main.cc
lib_gtest_a_CPPFLAGS = \
	$(GTEST_CPPFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)
libxorg_gtest_a_CXXFLAGS = $(GTEST_CXXFLAGS) $(AM_CXXFLAGS)

check_PROGRAMS = test_gtest

test_gtest_SOURCES = test-gtest.cpp

test_gtest_LDADD = \
	libgtest.a
