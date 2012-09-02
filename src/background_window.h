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

#define BACKGROUND_OPACITY 0.01

#ifdef _WIN32  
#  define BACKGROUND_UI_FILE "..\\share\\ardesia\\ui\\background_window.glade"
#else
#  define BACKGROUND_UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/background_window.glade"
#endif


/* Structure that contains the info passed to the callbacks. */
typedef struct
{

  /* Gtkbuilder for background window. */
  GtkBuilder *background_window_gtk_builder;

  /* 0 no background, 1 color, 1 image*/
  gint background_type;

  /* Background colour selected. */
  gchar *background_color; 

  /* Background image selected. */
  gchar *background_image;

  /* The background widget that represent the full window. */
  GtkWidget *background_window;

  /* cairo context to draw on the background window. */
  cairo_t *background_cr;

}BackgroundData;


/* Create the background window. */
GtkWidget *
create_background_window     ();

/* Set the background type. */
void
set_background_type     (gint type);


/* Get the background type */
gint
get_background_type     ();


/* Set the background image. */
void
set_background_image    (gchar *background_image);


/* Update the background image. */
void
update_background_image (gchar *name);


/* Get the background image */
gchar *
get_background_image    ();


/* Set the background colour. */
void
set_background_color    (gchar *rgba);


/* Update the background colour. */
void
update_background_color (gchar* rgba);


/* Get the background colour */
gchar *
get_background_color    ();


/* Clear the background. */
void
clear_background_window ();


/* Destroy background window. */
void
destroy_background_window    ();


/* Get the background window. */
GtkWidget *
get_background_window   ();


/* Set the background window. */
void
set_background_window   (GtkWidget *widget);


/* Upgrade the background window */
void
update_background       ();


