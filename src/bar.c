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

#include <utils.h>
#include <bar.h>


/* 
 * Calculate the better position where put the bar
 */
static void calculate_position(GtkWidget *ardesia_bar_window, 
			       gint dwidth, gint dheight, 
			       gint *x, gint *y, 
			       gint wwidth, gint wheight,
			       gint position)
{
  *y = ((dheight - wheight)/2); 
  /* vertical layout */
  if (position==WEST)
    {
      *x = 0;
    }
  else if (position==EAST)
    {
      *x = dwidth - wwidth;
    }
  else
    {
      /* horizontal layout */
      *x = (dwidth - wwidth)/2;
      if (position==NORTH)
        {
          *y = SPACE_FROM_BORDER; 
        }
      else if (position==SOUTH)
        {
	  /* south */
	  *y = dheight - SPACE_FROM_BORDER - wheight;
        }
      else
        {  
          /* invalid position */
          perror ("Valid positions are NORTH, SOUTH, WEST or EAST\n");
          exit(0);
        }
    }
}


/* 
 * Calculate the initial position
 * @TODO configuration file or wizard to decide the window position
 */
static void calculate_initial_position(GtkWidget *ardesia_bar_window, 
				       gint *x, gint *y, 
				       gint wwidth, gint wheight, 
				       gint position)
{
  gint dwidth = gdk_screen_width();
  gint dheight = gdk_screen_height();

  /* resize if larger that screen width */
  if (wwidth>dwidth)
    {
      wwidth = dwidth;
      gtk_window_resize(GTK_WINDOW(ardesia_bar_window), wwidth, wheight);       
    }

  /* resize if larger that screen height */
  if (wheight>dheight)
    {
      gint tollerance = 15;
      wheight = dheight - tollerance;
      gtk_widget_set_usize(ardesia_bar_window, wwidth, wheight);       
    }

  calculate_position(ardesia_bar_window, dwidth, dheight, x, y, wwidth, wheight, position);
}



/* Allocate and initialize the bar data structure */
static BarData* init_bar_data()
{
  BarData *bar_data = (BarData *) g_malloc((gsize) sizeof(BarData));
  bar_data->color = g_strdup_printf("%s", "FF0000FF");
  bar_data->annotation_is_visible = TRUE;
  bar_data->pencil = TRUE;
  bar_data->grab = TRUE;
  bar_data->text = FALSE;
  bar_data->thickness = THICK_STEP * 2;
  bar_data->highlighter = FALSE;
  bar_data->rectifier = FALSE;
  bar_data->rounder = FALSE;
  bar_data->arrow = FALSE;

  return bar_data;
}



/* Create the ardesia bar window */
GtkWidget* create_bar_window (CommandLine *commandline, GtkWidget *parent)
{
  GtkWidget *bar_window = NULL;
  GError* error = NULL;
  gchar* file = UI_FILE;

  gtkBuilder = gtk_builder_new();
  if (commandline->position>2)
    {
      /* north or south */
      file = UI_HOR_FILE;
    }
  else
    {
      /* east or west */
      if (gdk_screen_height() < 720)
        {
          /* 
           * the bar is too long and then I use an horizontal layout; 
           * this is done to have the full bar for netbook and screen
           * with low vertical resolution
           */
          file = UI_HOR_FILE;
          commandline->position=SOUTH;
        }
    }


  /* load the gtkbuilder file with the definition of the ardesia bar gui */
  gtk_builder_add_from_file(gtkBuilder, file, &error);
  if (error)
    {
      g_warning ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      g_object_unref (gtkBuilder);
      gtkBuilder = NULL;
      return bar_window;
    }  
  
  BarData *bar_data = init_bar_data();

  /* connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(gtkBuilder, (gpointer) bar_data);

  bar_window = GTK_WIDGET (gtk_builder_get_object(gtkBuilder, "winMain"));
  gtk_window_set_transient_for(GTK_WINDOW(bar_window), GTK_WINDOW(parent));
  if (commandline->decorated)
    {
      gtk_window_set_decorated(GTK_WINDOW(bar_window), TRUE);
    }
  gint width, height;
  gtk_window_get_size(GTK_WINDOW(bar_window) , &width, &height);

  /* x and y are the ardesia bar left corner coords */
  gint x, y;
  calculate_initial_position(bar_window, 
                             &x, &y, 
                             width, height,
                             commandline->position);

  /* the position is calculated respect the top left corner and then I set the north west gravity */
  gtk_window_set_gravity(GTK_WINDOW(bar_window), GDK_GRAVITY_NORTH_WEST);

  /* move the window in the desired position */
  gtk_window_move(GTK_WINDOW(bar_window), x, y);  

  return bar_window;
}
