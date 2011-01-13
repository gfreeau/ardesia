## Process this file with automake to produce Makefile.in

SUBDIRS = src po

ardesiadocdir = ${prefix}/doc/ardesia
ardesiadoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

autogen_FILE = autogen.sh

desktopdir = $(datadir)/applications
desktop_in_files = data/ardesia.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = data/pixmaps/ardesia.png

INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $(ardesiadoc_DATA) \
	$(INTLTOOL_FILES) $(autogen_FILE) \
	src/scripts src/pixmaps src/backgrounds\
	data

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

deb:
	debuild -us -uc -b
	mv ../$(PACKAGE_NAME)_$(PACKAGE_VERSION)-*.deb .

update-po:
	(cd po; $(MAKE) update-po)

clean:
	@for i in $(SUBDIRS); do \
        echo "Clearing in $$i..."; \
        (cd $$i; $(MAKE) clean); done
	rm -f  applications/ardesia.desktop
	rm -fr debian/tmp
	rm -fr debian/files
	rm -fr debian/ardesia
	rm -f  debian/debhelper.log
	rm -fr debian/substvars
	rm -f  ardesia/config.log

# Remove doc directory on uninstall
uninstall-local:
	rm -fr $(ardesiadocdir)
	rm -fr $(uidir)
	rm -fr $(datadir)/ardesia
