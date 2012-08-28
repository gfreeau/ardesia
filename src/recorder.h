/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <gtk/gtk.h>


#ifdef _WIN32
#  include <windows.h>
#  define RECORDER_FILE "..\\share\\ardesia\\scripts\\screencast.bat"
#else  
#  define RECORDER_FILE PACKAGE_DATA_DIR"/ardesia/scripts/screencast.sh"
#endif 


/*
 * Start the dialog that ask to the user where save the video
 * containing the screencast.
 * This function take as input the recorder tool button in ardesia bar
 * return true is the recorder is started.
 */
gboolean
start_save_video_dialog (GtkToolButton *toolbutton,
                         GtkWindow     *parent);


/* Quit the recorder. */
void
stop_recorder ();


/* Pause the recorder. */
void
pause_recorder ();


/* Resume the recorder. */
void
resume_recorder ();


/* Return if the recording is started. */
gboolean
is_started ();


/* Return if the recording is paused. */
gboolean
is_paused ();


/* Is the recorder available. */
gboolean
is_recorder_available ();


/* Missing program dialog. */
void
visualize_missing_recorder_program_dialog (GtkWindow *parent_window);


