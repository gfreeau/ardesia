
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


#include <utils.h>
#include <bar.h>

/* 
 * Calculate the better position where put the bar.
 */
static void
calculate_position (GtkWidget *ardesia_bar_window,
                    gint       d_width,
                    gint       d_height,
                    gint      *x,
                    gint      *y,
                    gint       w_width,
                    gint       w_height,
                    gint       position)
{
  *y = ((d_height - w_height - SPACE_FROM_BORDER)/2);
  /* Vertical layout. */
  if (position==WEST)
    {
      *x = 0;
    }
  else if (position==EAST)
    {
      *x = d_width - w_width;
    }
  else
    {
      /* Horizontal layout. */
      *x = (d_width - w_width)/2;
      if (position==NORTH)
        {
          *y = SPACE_FROM_BORDER;
        }
      else if (position==SOUTH)
        {
          /* South. */
          *y = d_height - SPACE_FROM_BORDER - w_height;
        }
      else
        {
          /* Invalid position. */
          perror ("Valid positions are NORTH, SOUTH, WEST or EAST\n");
          exit (EXIT_FAILURE);
        }
    }
}


/*
 * Calculate the initial position.
 */
static void
calculate_initial_position (GtkWidget *ardesia_bar_window,
                            gint *x,
                            gint *y,
                            gint w_width,
                            gint w_height,
                            gint position)
{
  gint d_width = gdk_screen_width ();
  gint d_height = gdk_screen_height ();

  /* Resize if larger that screen width. */
  if (w_width>d_width)
    {
      w_width = d_width;
      gtk_window_resize (GTK_WINDOW (ardesia_bar_window), w_width, w_height);
    }

  /* Resize if larger that screen height. */
  if (w_height>d_height)
    {
      gint tollerance = 15;
      w_height = d_height - tollerance;
      gtk_widget_set_size_request (ardesia_bar_window, w_width, w_height);
    }

  calculate_position (ardesia_bar_window, d_width, d_height, x, y, w_width, w_height, position);
}



/* Allocate and initialize the bar data structure. */
static BarData *
init_bar_data ()
{
  BarData *bar_data = (BarData *) g_malloc ((gsize) sizeof (BarData));
  bar_data->color = g_strdup_printf ("%s", "FF0000FF");
  bar_data->annotation_is_visible = TRUE;
  bar_data->grab = TRUE;
  bar_data->rectifier = FALSE;
  bar_data->rounder = FALSE;
  bar_data->thickness = MEDIUM_THICKNESS;

  return bar_data;
}

gchar*
get_xdg_config_file (const char *name)
{
  const gchar *user_dir = g_get_user_config_dir();
  const gchar* const *system_dirs;
  const gchar* const *dir;
  gchar *file;

  system_dirs = g_get_system_config_dirs();
  file = g_build_filename(user_dir, name, NULL);
  if (g_file_test(file, G_FILE_TEST_EXISTS) == TRUE)
  {
      return file;
  }
  
  free(file);

  for (dir = system_dirs; *dir; ++dir )
  {
      file = g_build_filename(*dir, name, NULL);
      if (g_file_test(file, G_FILE_TEST_EXISTS) == TRUE)
          return file;
          free(file);
  }
  return NULL;
}

/* Create the ardesia bar window. */
GtkWidget *
create_bar_window (CommandLine *commandline,
                   GtkWidget   *parent)
{
  GtkWidget *bar_window = (GtkWidget *) NULL;
  BarData *bar_data = (BarData *) NULL;
  GError *error = (GError *) NULL;
  gchar *file = UI_FILE;
  gint x = 0;
  gint y = 0;
  gint width = 0;
  gint height = 0;


  /* Set up style for ardesia */
  gchar* gtkcss_file = get_xdg_config_file("ardesia/gtk.css");
  if (gtkcss_file)
    {
      GtkCssProvider *css = gtk_css_provider_new ();
      gtk_css_provider_load_from_path (css, gtkcss_file, NULL);
      g_free (gtkcss_file);
      gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
                                                 GTK_STYLE_PROVIDER(css),
                                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    
  bar_gtk_builder = gtk_builder_new ();

  if (commandline->position>2)
    {
      /* North or south. */
      file = UI_HOR_FILE;
    }
  else
    {

      /* East or west. */
      if (gdk_screen_height () < 720)
        {
          /* 
           * The bar is too long and then I use an horizontal layout;
           * this is done to have the full bar for net book and screen
           * with low vertical resolution.
           */
          file = UI_HOR_FILE;
          commandline->position=SOUTH;
        }

    }


  /* Load the bar_gtk_builder file with the definition of the ardesia bar gui. */
  gtk_builder_add_from_file (bar_gtk_builder, file, &error);
  if (error)
    {
      g_warning ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      g_object_unref (bar_gtk_builder);
      bar_gtk_builder = NULL;
      return bar_window;
    }

  bar_data = init_bar_data ();

  bar_window = GTK_WIDGET (gtk_builder_get_object (bar_gtk_builder, "winMain"));
    
  /* Connect all the callback from bar_gtk_builder xml file. */
  gtk_builder_connect_signals (bar_gtk_builder, (gpointer) bar_data);

  //gtk_window_set_transient_for (GTK_WINDOW (bar_window), GTK_WINDOW (parent));

  if (commandline->decorated)
    {
      gtk_window_set_decorated (GTK_WINDOW (bar_window), TRUE);
    }

  gtk_window_get_size (GTK_WINDOW (bar_window) , &width, &height);

  /* x and y will be the bar left corner coordinates. */
  calculate_initial_position (bar_window,
                              &x,
                              &y,
                              width,
                              height,
                              commandline->position);

  /* The position is calculated respect the top left corner
   * and then I set the north west gravity. 
   */
  gtk_window_set_gravity (GTK_WINDOW (bar_window), GDK_GRAVITY_NORTH_WEST);

  /* Move the window in the desired position. */
  gtk_window_move (GTK_WINDOW (bar_window), x, y);


  return bar_window;
}


