## Process this file with automake to produce Makefile.in

AM_CFLAGS=-export-dynamic -Wall


INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = ardesia

ardesia_SOURCES = \
        pngutils.c pngutils.h\
	main.c \
	interface.c interface.h \
	callbacks.c

ardesia_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

