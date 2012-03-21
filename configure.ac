
AC_INIT(src/indicator-messages.c)

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-messages, 0.5.94)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([deprecations],
  [AS_HELP_STRING([--enable-deprecations],
    [allow deprecated API usage @<:@default=yes@:>@])],
  [],
  [enable_deprecations=yes])
AS_IF([test "x$enable_deprecations" = xno],
  [CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE -DGTK_DISABLE_SINGLE_INCLUDES"]
)

# the Google Test targets are cpp
AC_PROG_CXX

###########################
# Dependencies 
###########################

GTK_REQUIRED_VERSION=2.12
GTK3_REQUIRED_VERSION=3.0
GIO_UNIX_REQUIRED_VERSION=2.18
PANEL_REQUIRED_VERSION=2.0.0
INDICATE_REQUIRED_VERSION=0.6.90
INDICATOR_REQUIRED_VERSION=0.3.19
DBUSMENUGTK_REQUIRED_VERSION=0.5.90
GLIB_REQUIRED_VERSION=2.31.20

AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk],
    [Which version of gtk to use for the indicator @<:@default=3@:>@])],
  [],
  [with_gtk=3])

AS_IF([test "x$with_gtk" = x3],
        [PKG_CHECK_MODULES(APPLET, gtk+-3.0 >= $GTK3_REQUIRED_VERSION
                                   gio-unix-2.0 >= $GIO_UNIX_REQUIRED_VERSION
                                   indicator3-0.4 >= $INDICATOR_REQUIRED_VERSION
                                   indicate-0.7 >= $INDICATE_REQUIRED_VERSION
                                   dbusmenu-gtk3-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                   glib-2.0 >= $GLIB_REQUIRED_VERSION
                                   gmodule-2.0 >= $GLIB_REQUIRED_VERSION)
        ],
      [test "x$with_gtk" = x2],
        [PKG_CHECK_MODULES(APPLET, gtk+-2.0 >= $GTK_REQUIRED_VERSION
                                   gio-unix-2.0 >= $GIO_UNIX_REQUIRED_VERSION
                                   indicator-0.4 >= $INDICATOR_REQUIRED_VERSION
                                   indicate-0.7 >= $INDICATE_REQUIRED_VERSION
                                   dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                   glib-2.0 >= $GLIB_REQUIRED_VERSION
                                   gmodule-2.0 >= $GLIB_REQUIRED_VERSION)
        ],
      [AC_MSG_FAILURE([Value for --with-indicator-gtk was neither 2 nor 3])]
)
AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

###########################
# Test Dependencies
###########################

AC_ARG_ENABLE([tests],
	AC_HELP_STRING([--disable-tests], [Disable test scripts and tools]),,
	[enable_tests=auto])
AM_CONDITIONAL(BUILD_TESTS, test xyes = xyes)

###########################
# Status Provider Deps
###########################

PKG_CHECK_MODULES(STATUS_PROVIDER_PIDGIN, dbus-glib-1)
AC_SUBST(STATUS_PROVIDER_PIDGIN_CFLAGS)
AC_SUBST(STATUS_PROVIDER_PIDGIN_LIBS)

PKG_CHECK_MODULES(STATUS_PROVIDER_TELEPATHY, dbus-glib-1)
AC_SUBST(STATUS_PROVIDER_TELEPATHY_CFLAGS)
AC_SUBST(STATUS_PROVIDER_TELEPATHY_LIBS)

TELEPATHYGLIB_REQUIRED_VERSION=0.9.0
PKG_CHECK_MODULES(STATUS_PROVIDER_MC5, dbus-glib-1
                                       telepathy-glib >= $TELEPATHYGLIB_REQUIRED_VERSION)
AC_SUBST(STATUS_PROVIDER_MC5_CFLAGS)
AC_SUBST(STATUS_PROVIDER_MC5_LIBS)

PKG_CHECK_MODULES(STATUS_PROVIDER_EMESENE, dbus-glib-1)
AC_SUBST(STATUS_PROVIDER_EMESENE_CFLAGS)
AC_SUBST(STATUS_PROVIDER_EMESENE_LIBS)

###########################
# gcov coverage reporting
###########################

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

###########################
# Google Test framework
###########################

m4_include([m4/gtest.m4])
CHECK_GTEST

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/libindicate/icons/"
elif test "x$with_gtk" = x2; then
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator-0.4`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator-0.4`
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3-0.4`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3-0.4`
fi
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-messages
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default get text domain])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])

AM_GLIB_GNU_GETTEXT

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
src/indicator-messages-status-provider-0.5.pc.in
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/status/Makefile
data/icons/16x16/categories/Makefile
data/icons/22x22/Makefile
data/icons/22x22/status/Makefile
data/icons/22x22/categories/Makefile
data/icons/24x24/Makefile
data/icons/24x24/status/Makefile
data/icons/32x32/Makefile
data/icons/32x32/status/Makefile
data/icons/32x32/categories/Makefile
data/icons/48x48/Makefile
data/icons/48x48/status/Makefile
data/icons/scalable/Makefile
data/icons/scalable/status/Makefile
data/icons/scalable/categories/Makefile
po/Makefile.in
test/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

Messaging Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
	gcov:          $use_gcov
])
