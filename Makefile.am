## Process this file with automake to produce Makefile.in

SUBDIRS = src po

EXTRA_DIST = \
	autogen.sh 

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/backgrounds; then \
          $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/backgrounds; \
          for background in $(srcdir)/backgrounds/*; do \
            if test -f $$background; then \
              $(INSTALL_DATA) $$background $(DESTDIR)$(pkgdatadir)/backgrounds; \
            fi \
          done \
        fi  
	cp -p src/ardesia.ui $(DESTDIR)$(pkgdatadir); 
	cp -p src/preferenceDialog.ui $(DESTDIR)$(pkgdatadir); 

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi 
	if test -d backgrounds; then \
          mkdir $(distdir)/backgrounds; \
          for background in backgrounds/*; do \
            if test -f $$background; then \
              cp -p $$background $(distdir)/backgrounds; \
            fi \
          done \
        fi 
	cp -p src/ardesia.ui $(DESTDIR)$(pkgdatadir); 
	cp -p src/preferenceDialog.ui $(DESTDIR)$(pkgdatadir); 
