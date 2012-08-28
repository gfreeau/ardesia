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

#include <glib.h>

#include <gtk/gtk.h>

#include <cairo.h>

#ifdef _WIN32
#  define ANNOTATION_UI_FOLDER "..\\share\\ardesia\\ui"
#  define ANNOTATION_PIXMAPS_FOLDER "..\\share\\ardesia\\ui\\icons"
#  define ERASER_ICON ANNOTATION_PIXMAPS_FOLDER"\\eraser.png"
#  define PENCIL_ICON ANNOTATION_PIXMAPS_FOLDER"\\pencil.png"
#  define ARROW_ICON ANNOTATION_PIXMAPS_FOLDER"\\arrow.png"
#  define HIGHLIGHTER_ICON ANNOTATION_PIXMAPS_FOLDER"\\highlighter.png"
#else
#  define ANNOTATION_PIXMAPS_FOLDER PACKAGE_DATA_DIR"/ardesia/ui/icons"
#  define ERASER_ICON ANNOTATION_PIXMAPS_FOLDER"/eraser.png"
#  define PENCIL_ICON ANNOTATION_PIXMAPS_FOLDER"/pencil.png"
#  define ARROW_ICON ANNOTATION_PIXMAPS_FOLDER"/arrow.png"
#  define HIGHLIGHTER_ICON ANNOTATION_PIXMAPS_FOLDER"/highlighter.png"
#endif


/* Initialize the cursors variables. */
void 
cursors_main ();


/* Allocate a invisible cursor that can be used to hide the cursor icon. */
void
allocate_invisible_cursor (GdkCursor **cursor);


/* Set the pen cursor. */
void
set_pen_cursor (GdkCursor **cursor,
                gdouble thickness,
                gchar* color,
                gboolean arrow);


/* Set the eraser cursor. */
void
set_eraser_cursor (GdkCursor **cursor,
                   gint size);


/* Quit the cursors and free the inners variables. */
void 
cursors_main_quit ();




