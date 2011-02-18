man_MANS = ardesia.1
EXTRA_DIST = $(man_MANS)

install-data-hook:
	@list='$(man_MANS)';                              \
	for i in $$list; do                               \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`;      \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`;           \
	  f="$$n.$$s";                                    \
	  echo "cd $(DESTDIR)$(mandir)/man$$s";           \
	  cd $(DESTDIR)$(mandir)/man$$s;                  \
	  echo "rm -f $$f";                               \
	  rm -f $$f;                                      \
	  echo "ln -s $$i $$f";                           \
	  ln -s $$i $$f;                                  \
	done

uninstall-local:
	@list='$(man_MANS)';                              \
	for i in $$list; do                               \
	  s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`;      \
	  n=`echo $$i | sed -e 's/-[^-]*$$//'`;           \
	  f="$$n.$$s";                                    \
	  echo " rm -f $(DESTDIR)$(mandir)/man$$s/$$f";   \
	  rm -f $(DESTDIR)$(mandir)/man$$s/$$f;           \
	done
