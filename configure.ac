# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

m4_define([ardesia_version], [0.9])
m4_define([glib_required_version], [2.24.0])
m4_define([gtk_required_version], [2.16.0])
m4_define([gdk_pixbuf_required_version], [gtk_required_version])
m4_define([cairo_required_version], [1.8.0])
m4_define([cairo_pdf_required_version], [1.8.0])
m4_define([pangocairo_required_version], [1.22.0])
m4_define([libpng_required_version], [1.2.37])


AC_INIT([ardesia], [ardesia_version],
	http://code.google.com/p/ardesia/issues/list,
	[ardesia])

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_SRCDIR([src/ardesia.c])
AC_CONFIG_MACRO_DIR([m4macros])

AM_INIT_AUTOMAKE(no-define dist-bzip2)
AM_CONFIG_HEADER(config.h)

# Enable silent build rules by default, requires at least
# Automake-1.11. Disable by either passing --disable-silent-rules to
# configure or passing V=1 to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# gettext i18n support
GETTEXT_PACKAGE=ardesia
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

# Determine a C compiler to use
AC_PROG_CC
AM_PROG_CC_C_O

# Initialize libtool
AM_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AM_PROG_AS

# Initialize maintainer mode
AM_MAINTAINER_MODE

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"


# Check for pkg-config
PKG_PROG_PKG_CONFIG(0.16)

#################
# Check for Win32
#################
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac

######################
# Internationalisation
######################

IT_PROG_INTLTOOL([0.35.0],[no-xml])
AM_GLIB_GNU_GETTEXT

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
                libxml-2.0
		cairo
                gdk-2.0
                gtk+-2.0 >= 2.12"


PKG_CHECK_MODULES(ARDESIA, [$pkg_modules])

AC_CHECK_PROGS(XDG_UTILS, [xdg-mime xdg-icon-resource xdg-desktop-menu], [])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
docs/ardesia.1
docs/Makefile
src/Makefile
desktop/Makefile
win32/ardesia.iss
win32/build_installer
])

AC_OUTPUT
