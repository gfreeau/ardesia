## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	ardesia.desktop.in                \
        iwb-mime.xml                      \
	scripts                           \
	icons                             \
	backgrounds                       \
	$(ui_DATA)

uidir = $(datadir)/ardesia/ui
scriptsdir = $(datadir)/ardesia/scripts
uiiconsdir = $(uidir)/icons
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
pixmap_DATA = icons/ardesia.png


install-data-local:
	mkdir -p $(DESTDIR)$(uiiconsdir) 
	cp iwb-mime.xml $(DESTDIR)$(uidir) 
	cp -r icons/* $(DESTDIR)$(uiiconsdir) 
	mkdir -p $(DESTDIR)$(uibackgroundsdir) 
	cp -r backgrounds/* $(DESTDIR)$(uibackgroundsdir) 
	mkdir -p $(DESTDIR)$(scriptsdir) 
if !PLATFORM_WIN32
	cp -r scripts/*.sh $(DESTDIR)$(scriptsdir) 
else
	cp -r scripts/*.bat $(DESTDIR)$(scriptsdir) 
endif
	chmod a+x $(DESTDIR)$(scriptsdir)/*

	@if test x"$(XDG_UTILS)" != x; then                                                                                                          \
		echo "Register iwb mimetype";                                                                                                        \
		xdg-icon-resource install --mode user --novendor --context mimetypes --size 48 $(DESTDIR)$(uiiconsdir)/application-x-iwb.png;        \
		xdg-mime install --mode user $(DESTDIR)$(uidir)/iwb-mime.xml;                                                                        \
		xdg-desktop-menu install --mode user --novendor ardesia.desktop;                                                                     \
	fi

# Remove ui directory on uninstall
uninstall-local:
	rm -fr $(uiiconsdir) 
	rm -fr $(uibackgroundsdir) 
	@if test x"$(XDG_UTILS)" != x; then                                                                                                          \
		echo "Deregister iwb mimetype";                                                                                                        \
		xdg-icon-resource uninstall --mode user --novendor --context mimetypes --size 48 $(DESTDIR)$(uiiconsdir)/application-x-iwb.png;        \
		xdg-mime uninstall --mode user $(DESTDIR)$(uidir)/iwb-mime.xml;                                                                        \
		xdg-desktop-menu uninstall --mode user --novendor ardesia.desktop;                                                                     \
	fi

clean:
	rm -f ardesia.desktop
	rm -f  applications/ardesia.desktop
