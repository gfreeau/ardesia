dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_PREREQ(2.54)

m4_define([ardesia_version], 0.7)

m4_define([glib_required_version], [2.24.0])
m4_define([gtk_required_version], [2.20.0])
m4_define([gdk_pixbuf_required_version], [gtk_required_version])
m4_define([cairo_required_version], [1.8.0])
m4_define([cairo_pdf_required_version], [1.8.0])
m4_define([pangocairo_required_version], [1.22.0])
m4_define([libpng_required_version], [1.2.37])


AC_INIT(ardesia, [ardesia_version],
	http://code.google.com/p/ardesia/issues/list,
	[ardesia])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])


AM_INIT_AUTOMAKE(no-define dist-bzip2 no-dist-gzip)
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

AM_PATH_GLIB_2_0(glib_required_version, :,
  AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
  gobject gthread)

AM_PATH_GTK_2_0(gtk_required_version, :,
  AC_MSG_ERROR(Test for GTK+ failed. See the file 'INSTALL' for help.),
  gthread)

AC_MSG_CHECKING([if GTK+ is version 2.24.0 or newer])
if $PKG_CONFIG --atleast-version=2.24.0 gtk+-2.0; then
  have_gtk_2_24=yes
else
  have_gtk_2_24=no
fi
AC_MSG_RESULT($have_gtk_2_24)

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= gdk_pixbuf_required_version)

AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, no)
if test "x$GDK_PIXBUF_CSOURCE" = xno; then
  AC_MSG_ERROR(Could not find gdk-pixbuf-csource in your PATH)
fi

PKG_CHECK_MODULES(CAIRO, cairo >= cairo_required_version)

AC_CHECK_HEADER([sigsegv.h], AC_CHECK_LIB([sigsegv], [stackoverflow_install_handler]), AC_MSG_NOTICE([*** Header file sigsegv.h not found; please install the libsigsegv headers ***])) 
AC_CHECK_HEADER([bfd.h], AC_CHECK_LIB([bfd], [bfd_openr]), AC_MSG_NOTICE([*** Header file bfd.h not found; please install the binutils headers  ***])) 
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
