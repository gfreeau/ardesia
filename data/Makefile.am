## Process this file with automake to produce Makefile.in

EXTRA_DIST = scripts pixmaps backgrounds $(ui_DATA)

uidir = $(datadir)/ardesia/ui
scriptsdir = $(datadir)/ardesia/scripts
uipixmapsdir = $(uidir)/pixmaps
uibackgroundsdir = $(uidir)/backgrounds
ui_DATA = vertical_bar.glade \
	  horizontal_bar.glade \
	  preference_dialog.glade \
	  annotation_window.glade \
	  background_window.glade \
	  text_window.glade \
	  info_dialog.glade

desktopdir = $(datadir)/applications
desktop_in_files = ardesia.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = pixmaps/ardesia.png


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

clean:
	rm -f ardesia.desktop
	rm -f  applications/ardesia.desktop
