dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(ardesia, 0.5)

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_MAINTAINER_MODE


LT_PREREQ([2.2.6])
LT_INIT([dlopen win32-dll disable-static])
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(DLLTOOL, dlltool)


dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
GETTEXT_PACKAGE=ardesia
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([0.35.0])
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT

case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac

AM_PROG_LIBTOOL

AC_CHECK_LIB([sigsegv], [stackoverflow_install_handler])
AC_CHECK_HEADER(sigsegv.h, , AC_MSG_ERROR([SISEGV header not found; please install the libsigsegv-dev package.]))
AC_CHECK_LIB(iberty, dyn_string_append)
AC_CHECK_LIB(bfd, bfd_openr)
AC_CHECK_HEADER(bfd.h, , AC_MSG_ERROR([BFD header not found; please install the binutils-dev package.]))
AC_CHECK_FUNCS(backtrace)

AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = yes)

dnl Check modules
pkg_modules="	glib-2.0
                gthread-2.0
                gsl
		cairo
                gdk-2.0
                gtk+-2.0 >= 2.12"


PKG_CHECK_MODULES(ARDESIA, [$pkg_modules])

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
