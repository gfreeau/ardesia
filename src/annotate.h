/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


/* Initialize the annotation window */
int annotate_init (int x, int y, int width, int height);

/* Get the annotation window */
GtkWindow* get_annotation_window();

/* Hide the annotations */
void annotate_hide_annotation ();

/* Show the annotations */
void annotate_show_annotation ();

/* Get cairo context that contains the annotations */
cairo_t* get_annotation_cairo_context();

/* Undo to the last save point */
void annotate_undo();

/* Quit the annotation */
void annotate_quit();

/* Set the pen color */
void annotate_set_color(gchar* color);

/* Set line width */
void annotate_set_width(guint width);

/* Set arrow type */
void annotate_set_arrow(int arrow);

/* Start to paint */
void annotate_toggle_grab ();

/* Start to erase */
void annotate_eraser_grab ();

/* Release pointer grab */
void annotate_release_grab ();

/* Acquire pointer grab */
void annotate_acquire_grab ();

/* Clear the annotations windows */
void annotate_clear_screen ();


