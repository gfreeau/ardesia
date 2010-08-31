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


#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gtk/gtk.h>


typedef struct
{
  gchar* background_color; 
  gchar* background_image; 
  GtkWidget* background_window;
  /* cairo context to draw on the background window*/
  cairo_t *back_cr;
  /* used for input shape mask */
  GdkPixmap* background_shape; 
}BackGroundData;


/* Create the background window */
GtkWidget* create_background_window();

/* Change the background image */
void change_background_image(gchar *backgroundimage);

/* Change the background color */
void change_background_color(gchar *rgba);

/* Clear the background */
void clear_background_window();

/** Destroy background window */
void destroy_background_window();

/* Get the background window */
GtkWidget* get_background_window();

/* Set the background window */
void set_background_window(GtkWidget* widget);


