dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(ardesia, 0.3)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC



dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
GETTEXT_PACKAGE=ardesia
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])


dnl Libraries are dynamic by default.
dnl User can override by passing the flag --enable-static
AC_DISABLE_STATIC

AM_PROG_LIBTOOL

case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac

AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = yes)

dnl See if there is the static linking option
AM_CONDITIONAL(ENABLE_STATIC, test "$enable_static" = "yes")
	

dnl Check modules
pkg_modules="	gtk+-2.0 >= 2.12 
		cairo
                gsl"

PKG_CHECK_MODULES(ARDESIA, [$pkg_modules])

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
