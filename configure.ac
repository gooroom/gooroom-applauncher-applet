dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_INIT([gooroom-applauncher-applet], [0.1.0])
AC_PREREQ([2.63])

dnl ***********************
dnl Initialize automake ***
dnl ***********************
AM_INIT_AUTOMAKE([1.8 dist-xz no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_LD()
AC_PROG_INSTALL()
IT_PROG_INTLTOOL([0.35.2])

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.4])
LT_INIT([disable-static])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE=gooroom-applauncher-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AM_GLIB_GNU_GETTEXT

dnl ***************************
dnl *** check for GSettings ***
dnl ***************************
GLIB_GSETTINGS

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.20.0)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.44.0)
PKG_CHECK_MODULES(GNOME_APPLETS, libpanel-applet >= 3.24.1)
PKG_CHECK_MODULES(GNOME_MENU, libgnome-menu-3.0)
PKG_CHECK_MODULES(STARTUP_NOTIFICATION, libstartup-notification-1.0 >= 0.12)


AC_ARG_WITH([libpanel-applet-dir], [], [LIBPANEL_APPLET_DIR=$withval], [LIBPANEL_APPLET_DIR=""])
if test "$LIBPANEL_APPLET_DIR" == ""; then
	LIBPANEL_APPLET_DIR=`$PKG_CONFIG --variable=libpanel_applet_dir libpanel-applet`
fi

AC_MSG_NOTICE([installing applets in $LIBPANEL_APPLET_DIR])
AC_SUBST(LIBPANEL_APPLET_DIR)


AC_OUTPUT([
  Makefile
  src/Makefile
  data/Makefile
  po/Makefile.in
])
