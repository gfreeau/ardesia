## Process this file with automake to produce Makefile.in


if PLATFORM_WIN32
        AM_CPPFLAGS = -DPACKAGE_LOCALE_DIR=\""../$(DATADIRNAME)/locale"\" 
else
        AM_CPPFLAGS = -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 
endif

AM_CPPFLAGS += -DPACKAGE_SRC_DIR=\""$(srcdir)"\"  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"       \
	$(ARDESIA_CFLAGS) 

AM_CFLAGS =\
	 -Wall                                    \
	 -g -L$(libdir)

bin_PROGRAMS = ardesia

ardesia_SOURCES = \
	bar.c                                     \
	bar.h                                     \
	bar_callbacks.c                           \
	bar_callbacks.h                           \
	annotation_window_callbacks.c             \
	annotation_window_callbacks.h             \
	background_window.c                       \
	background_window.h                       \
	background_window_callbacks.c             \
	background_window_callbacks.h             \
        broken.c                                  \
	broken.h                                  \
        bezier_spline.c                           \
	bezier_spline.h                           \
        crash_dialog.c                            \
	crash_dialog.h                            \
	crash_dialog_callbacks.c                  \
	crash_dialog_callbacks.h                  \
	cursors.c                                 \
	cursors.h                                 \
        pdf_saver.c                               \
	pdf_saver.h                               \
        annotation_window.c                       \
	annotation_window.h                       \
        utils.c                                   \
	utils.h                                   \
        input.c                                   \
	input.h                                   \
        windows_utils.c                           \
	windows_utils.h                           \
	keyboard.c                                \
	keyboard.h                                \
        saver.c                                   \
	saver.h                                   \
        color_selector.c                          \
	color_selector.h                          \
	iwb_saver.c                               \
	iwb_saver.h                               \
	iwb_loader.c                              \
	iwb_loader.h                              \
        preference_dialog.c                       \
	preference_dialog.h                       \
	preference_dialog_callbacks.c             \
	preference_dialog_callbacks.h             \
        project_dialog.c                          \
	project_dialog.h                          \
	project_dialog_callbacks.c                \
	project_dialog_callbacks.h                \
        share_confirmation_dialog.c               \
	share_confirmation_dialog.h               \
	share_confirmation_dialog_callbacks.c     \
	share_confirmation_dialog_callbacks.h     \
        text_window.c                             \
	text_window.h                             \
        info_dialog.c                             \
	info_dialog.h                             \
        recorder.c                                \
	recorder.h                                \
	ardesia.c                                 \
	ardesia.h 
   

ardesia_LDFLAGS = 

if !PLATFORM_WIN32
if WITH_GNU_LD
ardesia_LDFLAGS += -Wl,--export-dynamic
endif
else
#This create a windows executable instead an msdos ones
ardesia_LDFLAGS += -mwindows -lbfd -lintl -liberty -limagehlp -lole32 -luuid
endif

ardesia_LDADD = $(ARDESIA_LIBS)
