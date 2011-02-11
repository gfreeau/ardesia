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
 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <utils.h> 
#include <background_window.h>
#include <annotation_window.h>


/* The background data used internally and by the callbacks. */
static BackGroundData *background_data;


/* Load the "filename" file content into the image buffer. */
static GdkPixbuf *
load_png (gchar *filename)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  GdkPixbuf *scaled = NULL;

  if (pixbuf)
    {
      gint height;
      gint width;
      gdk_drawable_get_size (background_data->background_window->window, &width, &height);
      scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
      g_object_unref (G_OBJECT (pixbuf));
    }
  else
    {
      /* @TODO Handle this in a visual way or avoid this. */
      g_printerr ("Failed to load the file %s as background\n", filename);
      exit (EXIT_FAILURE);
    }
  pixbuf = scaled;
  return pixbuf;
}


/* Load a file image in the window. */
static void
load_file ()
{
  if (background_data->back_cr)
    {
      GdkPixbuf* pixbuf = load_png (background_data->background_image);   
      cairo_set_operator (background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      gtk_window_set_opacity (GTK_WINDOW (background_data->background_window), 1);
      gdk_cairo_set_source_pixbuf (background_data->back_cr, pixbuf, 0.0, 0.0);
      cairo_paint (background_data->back_cr);
      cairo_stroke (background_data->back_cr);   
      g_object_unref (G_OBJECT (pixbuf));
#ifndef _WIN32
      gdk_window_input_shape_combine_mask (background_data->background_window->window,
					   NULL, 
					   0, 0);
#endif
    }
}


/* The windows has been exposed after the show_all request to change the background color. */
static void
load_color ()
{
  gint r,g,b,a;
  if (background_data->back_cr)
    {
      cairo_set_operator (background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      sscanf (background_data->background_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
      /*
       * @TODO Implement with a full opaque windows and use cairo_set_source_rgba 
       * function to paint.
       * I set the opacity with alpha and I use cairo_set_source_rgb to workaround 
       * the problem on windows with rgba. 
       */
      gtk_window_set_opacity (GTK_WINDOW (background_data->background_window), (gdouble) a/256);
      cairo_set_source_rgb (background_data->back_cr, (gdouble) r/256, (gdouble) g/256, (gdouble) b/256);
      cairo_paint (background_data->back_cr);
      cairo_stroke (background_data->back_cr);
      
#ifndef _WIN32
      gdk_window_input_shape_combine_mask (background_data->background_window->window,
					   NULL,
					   0,
					   0);
#endif
      
    }  
}


/* Allocate internal structure. */
static BackGroundData* allocate_background_data ()
{
  BackGroundData* background_data   = g_malloc ((gsize) sizeof (BackGroundData));
  background_data->background_color = NULL; 
  background_data->background_image = NULL; 
  background_data->back_cr          = NULL;
  background_data->background_shape = NULL; 
  background_data->background_image = NULL;
  background_data->background_window = NULL;
  return background_data;
}


/* Destroy the background window. */
void
destroy_background_window ()
{
  if (background_data)
    {
      if (background_data->background_shape)
	{
	  g_object_unref (background_data->background_shape);
          background_data->background_shape = NULL;
	}

      if (background_data->background_window)
	{ 
	  /* Destroy brutally the background window. */
	  gtk_widget_destroy (background_data->background_window);
	  background_data->background_window = NULL;
	}

      if (background_data->back_cr)
	{
	  cairo_destroy (background_data->back_cr);
          background_data->back_cr = NULL;
	}

      if (background_data->background_color)
	{
	  g_free (background_data->background_color);
	  background_data->background_color = NULL;
	}

      /* Unref gtkbuilder. */
      if (background_data->background_window_gtk_builder)
	{
	  g_object_unref (background_data->background_window_gtk_builder);
	  background_data->background_window_gtk_builder = NULL;
	}

      if (background_data)
	{
	  g_free (background_data);
	  background_data = NULL;
	}

    }
  /* Quit the gtk engine. */
  gtk_main_quit ();
}


/* Clear the background. */
void clear_background_window ()
{
  gint height       = -1;
  gint width        = -1;
  cairo_t *shape_cr = NULL;
  
  if (background_data->background_color)
    {
      g_free (background_data->background_color);
      background_data->background_color = NULL;
    }

  if (background_data->background_image)
    {
      g_free (background_data->background_image);
      background_data->background_image = NULL;
    }  

  /* 
   * @HACK Deny the mouse input to go below the window putting the opacity greater than 0
   * I avoid a complete transparent window because in some operating system this would become
   * transparent to the pointer input also.
   *
   */
  gtk_window_set_opacity (GTK_WINDOW (background_data->background_window), BACKGROUND_OPACITY);

  clear_cairo_context (background_data->back_cr);
  
  gdk_drawable_get_size (background_data->background_window->window, &width, &height);

  /* Instantiate a transparent pixmap to be used as mask. */
  background_data->background_shape = gdk_pixmap_new (NULL, width, height, 1); 
  shape_cr = gdk_cairo_create (background_data->background_shape);
  clear_cairo_context (shape_cr); 
  cairo_destroy (shape_cr);

  /* This allows the mouse event to be passed to the window below. */
#ifndef _WIN32
  gdk_window_input_shape_combine_mask (background_data->background_window->window,
				       background_data->background_shape,
				       0,
				       0);
#endif
}


/* Create the background window. */
GtkWidget* 
create_background_window ()
{
  GError *error = NULL;
  
  background_data = allocate_background_data (); 
  
  /* Initialize the background window. */
  background_data->background_window_gtk_builder = gtk_builder_new ();

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (background_data->background_window_gtk_builder, BACKGROUND_UI_FILE, &error);

  if (error)
    {
      g_warning ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      return background_data->background_window;
    }  
 
  GObject *background_obj = gtk_builder_get_object (background_data->background_window_gtk_builder, "backgroundWindow");
  background_data->background_window = GTK_WIDGET (background_obj); 

  gtk_window_set_opacity (GTK_WINDOW (background_data->background_window), BACKGROUND_OPACITY);

  gtk_widget_set_usize (background_data->background_window, gdk_screen_width (), gdk_screen_height ());

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals (background_data->background_window_gtk_builder, (gpointer) background_data);

  gtk_widget_show_all (background_data->background_window);

  /* This put in full screen; this will generate an exposure. */
  gtk_window_fullscreen (GTK_WINDOW (background_data->background_window));

#ifdef _WIN32
  /* In the gtk 2.16.6 used for windows the gtkbuilder the double buffered property 
   * is not parsed from glade and then I set this by hands. 
   */
  gtk_widget_set_double_buffered (background_data->background_window, FALSE);
#endif
  
  return  background_data->background_window;
}


/* Change the background image of ardesia. */
void
change_background_image (gchar *name)
{
  if (background_data->background_color)
    {
      g_free (background_data->background_color);
      background_data->background_color = NULL;
    }

  background_data->background_image = name;
  load_file ();
}


/* Change the background colour of ardesia. */
void
change_background_color (gchar* rgba)
{
  if (background_data->background_image)
    {
      g_free (background_data->background_image);
      background_data->background_image = NULL;
    }  

  if (!background_data->background_color)
    {
      background_data->background_color = g_strdup_printf ("%s", rgba);
    }

  load_color ();  
}


/* Get the background window. */
GtkWidget *
get_background_window ()
{
  return background_data->background_window;
}


/* Set the background window. */
void
set_background_window (GtkWidget* widget)
{
  background_data->background_window = widget;
}


