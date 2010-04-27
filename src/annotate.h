/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
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


/* Struct to store the painted point */
typedef struct
{
  gint x;
  gint y;
  gint width;
} AnnotateStrokeCoordinate;


/* Initialize the annotation window */
int annotate_init (int x, int y, int width, int height, gboolean debug);

/* Get the annotation window */
GtkWidget* get_annotation_window();

/* Hide the annotations */
void annotate_hide_annotation ();

/* Show the annotations */
void annotate_show_annotation ();

/* Get cairo context that contains the annotations */
cairo_t* get_annotation_cairo_context();

/* Get the cairo context that contains the background */
cairo_t* get_annotation_cairo_background_context();

/* Set the cairo context that contains the background */
void set_annotation_cairo_background_context(cairo_t* background_cr);

/* Undo to the last save point */
void annotate_undo();

/* Redo to the last save point */
void annotate_redo();

/* Quit the annotation */
void annotate_quit();

/* Set the pen color */
void annotate_set_color(gchar* color);

/* Set line width */
void annotate_set_width(guint width);

/* Set rectifier */
void annotate_set_rectifier(gboolean rectify);

/* Set rounder */
void annotate_set_rounder(gboolean rounder);

/* fill the last shape if it is a close path */
void annotate_fill();

/* Set arrow */
void annotate_set_arrow(gboolean arrow);

/* Start to paint */
void annotate_toggle_grab();

/* Start to erase */
void annotate_eraser_grab();

/* Release pointer grab */
void annotate_release_grab();

/* Acquire pointer grab */
void annotate_acquire_grab();

/* Clear the annotations windows */
void annotate_clear_screen();

/* Paint the context over the annotation window */
void merge_context(cairo_t * cr);


