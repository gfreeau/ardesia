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


#include <config.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <ardesia.h>


#ifdef _WIN32
#  include <windows_utils.h>
#  define UI_FILE "..\\share\\ardesia\\ui\\vertical_bar.glade"
#  define UI_HOR_FILE "..\\share\\ardesia\\ui\\horizontal_bar.glade"
#else
#  define UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/vertical_bar.glade"
#  define UI_HOR_FILE PACKAGE_DATA_DIR"/ardesia/ui/horizontal_bar.glade"
#endif


/* Distance space from border to the ardesia bar in pixel unit. */
#define SPACE_FROM_BORDER 35

#define MICRO_THICKNESS   3
#define THIN_THICKNESS    6
#define MEDIUM_THICKNESS  12
#define THICK_THICKNESS   18    


/* Structure that contains the info passed to the callbacks. */
typedef struct
{

  /* rectifier flag. */
  gboolean rectifier;

  /* rounder flag. */
  gboolean rounder;

  /* selected colour in RGBA format. */
  gchar* color;

  /* selected line thickness. */
  gint thickness;

  /* annotation is visible. */
  gboolean annotation_is_visible;

  /* grab when leave. */
  gboolean grab;

}BarData;


/* Create the ardesia bar window. */
GtkWidget *
create_bar_window (CommandLine *commandline,
                   GtkWidget   *parent);


