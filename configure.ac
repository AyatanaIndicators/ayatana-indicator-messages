
AC_INIT(src/indicator-messages.c)

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-messages, 0.2.2)

AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

###########################
# Dependencies 
###########################

GTK_REQUIRED_VERSION=2.12
GIO_UNIX_REQUIRED_VERSION=2.18
PANEL_REQUIRED_VERSION=2.0.0
INDICATE_REQUIRED_VERSION=0.2.0
INDICATOR_REQUIRED_VERSION=0.2.0
DBUSMENUGTK_REQUIRED_VERSION=0.1.1

PKG_CHECK_MODULES(APPLET, gtk+-2.0 >= $GTK_REQUIRED_VERSION
                          gio-unix-2.0 >= $GIO_UNIX_REQUIRED_VERSION
                          indicator >= $INDICATOR_REQUIRED_VERSION
                          indicate >= $INDICATE_REQUIRED_VERSION
                          dbusmenu-gtk >= $DBUSMENUGTK_REQUIRED_VERSION)
AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

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
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
fi
AC_SUBST(INDICATORDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/status/Makefile
data/icons/22x22/Makefile
data/icons/22x22/status/Makefile
data/icons/24x24/Makefile
data/icons/24x24/status/Makefile
data/icons/32x32/Makefile
data/icons/32x32/status/Makefile
data/icons/48x48/Makefile
data/icons/48x48/status/Makefile
data/icons/scalable/Makefile
data/icons/scalable/status/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

Messaging Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
])
