## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/ardesia/ui
scriptsdir = $(datadir)/ardesia/scripts
uipixmapsdir = $(uidir)/pixmaps
uibackgroundsdir = $(uidir)/backgrounds
ui_DATA = ardesia.glade \
	ardesia_horizontal.glade \
	preference_dialog.glade \
	info_dialog.glade

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(ARDESIA_CFLAGS) $(GCONF_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g -L$(libdir)

bin_PROGRAMS = ardesia

ardesia_SOURCES = \
	bar_callbacks.c \
	bar_callbacks.h \
	annotate_callbacks.c \
	annotate_callbacks.h \
	background.c \
	background.h \
        broken.c \
	broken.h \
        bezier_spline.c \
	bezier_spline.h \
        annotate.c \
	annotate.h \
        utils.c \
	utils.h \
        windows_utils.c \
	windows_utils.h \
        saver.c \
	saver.h \
        color_selector.c \
	color_selector.h \
        preference_dialog.c \
	preference_dialog.h \
        text_widget.c \
	text_widget.h \
        info_dialog.c \
	info_dialog.h \
        recorder.c \
	recorder.h \
	ardesia.c \
        ardesia.h

if ENABLE_STATIC
ardesia_LDFLAGS = -static-libtool-libs
else
if WITH_GNU_LD
ardesia_LDFLAGS = -Wl,--export-dynamic
endif
endif

ardesia_LDADD = $(ARDESIA_LIBS) $(GCONF_LIBS)

EXTRA_DIST = $(ui_DATA)

install-data-local:
	mkdir -p $(DESTDIR)$(uipixmapsdir) 
	cp -r pixmaps/* $(DESTDIR)$(uipixmapsdir) 
	mkdir -p $(DESTDIR)$(uibackgroundsdir) 
	cp -r backgrounds/* $(DESTDIR)$(uibackgroundsdir) 
	mkdir -p $(DESTDIR)$(scriptsdir) 
	cp -r scripts/* $(DESTDIR)$(scriptsdir) 
	chmod a+x $(DESTDIR)$(scriptsdir)/* 

# Remove ui directory on uninstall
uninstall-local:
	rm -fr $(uipixmapsdir) 
	rm -fr $(uibackgroundsdir) 
	
