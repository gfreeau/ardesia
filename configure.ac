# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

m4_define([ardesia_version], [1.1])
m4_define([gtk_required_version], [2.99.3])

AC_INIT([ardesia], [ardesia_version],
	http://code.google.com/p/ardesia/issues/list,
	[ardesia])

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_MACRO_DIR([m4macros])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

# Check if we have enabled debug support.
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug          enable debug build [[default=no]]],, enable_debug="no")
if test "x$enable_debug" = "xyes"; then
	CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -DDEBUG"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

# gettext i18n support
GETTEXT_PACKAGE=ardesia
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

# Determine a C compiler to use
AC_ISC_POSIX
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_HEADER_STDC

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

#######
# Gtk 3
#######
AM_PATH_GTK_3_0(gtk_required_version, :,
  AC_MSG_ERROR(Test for GTK3+ failed. See the file 'INSTALL' for help.),
  gthread)

AC_MSG_CHECKING([if GTK+ is version 2.99.3 or newer])
if $PKG_CONFIG --atleast-version=2.99.3 gtk+-3.0; then
  have_gtk3=yes
else
  have_gtk3=no
fi

AC_MSG_RESULT($have_gtk_3)

AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = yes)

################
# Check packages
################
PKG_CHECK_MODULES(ARDESIA, [gtk+-3.0 gsl libgsf-1])
AC_CHECK_PROGS(XDG_UTILS, [xdg-mime xdg-icon-resource xdg-desktop-menu], [])
AC_SUBST(GTK3_CFLAGS)
AC_SUBST(GTK3_LIBS)

# ***************************************************************************
# finally
# ***************************************************************************
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
