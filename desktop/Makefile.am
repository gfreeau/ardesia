## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	ardesia.desktop.in                \
	scripts                           \
	pixmaps                           \
	backgrounds                       \
	$(ui_DATA)

uidir = $(datadir)/ardesia/ui
scriptsdir = $(datadir)/ardesia/scripts
uipixmapsdir = $(uidir)/pixmaps
uibackgroundsdir = $(uidir)/backgrounds

ui_DATA = vertical_bar.glade              \
	  horizontal_bar.glade            \
	  preference_dialog.glade         \
	  project_dialog.glade            \
	  crash_dialog.glade              \
	  annotation_window.glade         \
	  background_window.glade         \
	  share_confirmation_dialog.glade \
	  text_window.glade               \
	  info_dialog.glade

desktopdir = $(datadir)/applications
desktop_in_files = ardesia.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = pixmaps/ardesia.png


install-data-local:
	mkdir -p $(DESTDIR)$(uipixmapsdir) 
	cp iwb-mime.xml $(DESTDIR)$(uidir) 
	cp -r pixmaps/* $(DESTDIR)$(uipixmapsdir) 
	mkdir -p $(DESTDIR)$(uibackgroundsdir) 
	cp -r backgrounds/* $(DESTDIR)$(uibackgroundsdir) 
	mkdir -p $(DESTDIR)$(scriptsdir) 
	cp -r scripts/* $(DESTDIR)$(scriptsdir) 
	chmod a+x $(DESTDIR)$(scriptsdir)/*

	@if test x"$(XDG_UTILS)" != x; then                                                                                        \
		echo "Register iwb mimetype";                                                                                      \
		xdg-icon-resource install --context mimetypes --mode system --novendor --size 48 pixmaps/application-x-iwb.png;    \
		xdg-mime install --mode system /usr/local/share/applications/iwb-mime.xml;                                         \	
		xdg-desktop-menu install --mode system --novendor /usr/local/share/applications/ardesia.desktop;                   \
	fi

# Remove ui directory on uninstall
uninstall-local:
	rm -fr $(uipixmapsdir) 
	rm -fr $(uibackgroundsdir) 

clean:
	rm -f ardesia.desktop
	rm -f  applications/ardesia.desktop
