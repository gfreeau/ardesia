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


/* initialize the annotation */
int annotate_init (int x, int y, int width, int height);

/* load annotation window */
GtkWindow* get_annotation_window();

/* hide the  window with the annotations */
void annotate_hide_window ();

/* show the window with the annotations */
void annotate_show_window ();

/* quit the annotation */
void annotate_quit();

/* set the pen color */
void annotate_set_color(gchar* color);

/* set line width */
void annotate_set_width(guint width);

/* set arrow type */
void annotate_set_arrow(int arrow);

/* start to paint */
void annotate_toggle_grab ();

/* start to erase */
void annotate_eraser_grab ();

/* release pointer grab */
void annotate_release_grab ();

/* acquire pointer grab */
void annotate_acquire_grab ();

/* clear the annotations windows */
void annotate_clear_screen ();


