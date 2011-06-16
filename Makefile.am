## Process this file with automake to produce Makefile.in

SUBDIRS = \
	desktop                        \
	docs                           \
	po                             \
	src 

ardesiadocdir = ${prefix}/share/doc/ardesia
ardesiadoc_DATA =                      \
	README                         \
	AUTHORS                        \
	ChangeLog                      \
	NEWS

autogen_FILE = autogen.sh

@INTLTOOL_DESKTOP_RULE@

INTLTOOL_FILES = intltool-extract.in   \
	intltool-merge.in              \
	intltool-update.in

EXTRA_DIST = $(ardesiadoc_DATA)        \
	$(INTLTOOL_FILES)              \
	$(autogen_FILE)                \
	win32 

DISTCLEANFILES = intltool-extract      \
	intltool-merge                 \
	intltool-update                \
	po/.intltool-merge-cache


.PHONY : clean win-installer

if !PLATFORM_WIN32

deb:
	@if test -s "`which debuild`"; then                                                \
		debuild -us -uc -b;                                                        \
		mv ../$(PACKAGE_NAME)_$(PACKAGE_VERSION)-*_*.deb .;                        \
	else                                                                               \
		echo "You need to install the debian package 'devscripts'";                \
		exit 1;                                                                    \
	fi

else
win-installer:
	@win32/build_installer
	
endif
	
update-po:
	(cd po; $(MAKE) update-po)

clean:
	@for i in $(SUBDIRS); do       \
        echo "Clearing in $$i...";     \
        (cd $$i; $(MAKE) clean); done
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
