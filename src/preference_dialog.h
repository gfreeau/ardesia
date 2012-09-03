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
#  define PREFERENCE_UI_FILE "..\\share\\ardesia\\ui\\preference_dialog.glade"
#  define BACKGROUNDS_FOLDER "..\\share\\ardesia\\ui\\backgrounds"
#else
#  define PREFERENCE_UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/preference_dialog.glade"
#  define BACKGROUNDS_FOLDER PACKAGE_DATA_DIR"/ardesia/ui/backgrounds"
#endif


typedef struct
{

  /* Preference dialog. */
  GtkBuilder *preference_dialog_gtk_builder;

  /* Preview of background file. */
  GtkWidget *preview;

}PreferenceData;


/* Show the permission denied to access to file dialog. */
void
show_permission_denied_dialog     (GtkWindow  *parent_window);


/*
 * Start the dialog that ask to the user
 * the background setting.
 */
void
start_preference_dialog           (GtkWindow  *parent);


