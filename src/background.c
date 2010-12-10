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
#include <config.h>
#endif

#include <background.h>
#include <utils.h> 
#include <annotate.h>


static BackGroundData* background_data;


/* Load the "filename" file content into the pixbuf */
static GdkPixbuf * load_png (gchar *filename)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  GdkPixbuf *scaled = NULL;

  if (pixbuf)
    {
      gint height;
      gint width;
      gdk_drawable_get_size(background_data->background_window->window, &width, &height);
      scaled = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
      g_object_unref(G_OBJECT (pixbuf));
    }
  else
    {
      /* @TODO Handle this in a visual way or avoid this */
      g_printerr("Couldn't load the file %s as background\n", filename);
      exit(0);
    }
  pixbuf = scaled;
  return pixbuf;
}


/* Load a file image in the window */
static void load_file()
{
  if (background_data->back_cr)
    {
      GdkPixbuf* pixbuf = load_png(background_data->background_image);   
      cairo_set_operator(background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), 1);
      gdk_cairo_set_source_pixbuf(background_data->back_cr, pixbuf, 0.0, 0.0);
      cairo_paint(background_data->back_cr);
      cairo_stroke(background_data->back_cr);   
      g_object_unref(G_OBJECT(pixbuf));
#ifndef _WIN32
      gdk_window_input_shape_combine_mask(background_data->background_window->window,
					  NULL, 
					  0, 0);
#endif
    }
}


/* The windows has been exposed after the show_all request to change the background color */
static void load_color()
{
  if (background_data->back_cr)
    {
      cairo_set_operator(background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      gint r,g,b,a;
      sscanf(background_data->background_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
      /*
       * @TODO implement with a full opaque windows and use cairo_set_source_rgba function to paint
       * I set the opacity with alpha and I use cairo_set_source_rgb to workaround the problem on windows with rgba 
       *
       d*/
      gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), (gdouble) a/256);
      cairo_set_source_rgb(background_data->back_cr, (gdouble) r/256, (gdouble) g/256, (gdouble) b/256);
      cairo_paint(background_data->back_cr);
      cairo_stroke(background_data->back_cr);
      
#ifndef _WIN32
      gdk_window_input_shape_combine_mask(background_data->background_window->window,  
					  NULL, 
					  0, 0);
#endif
      
    }  
}


/* Allocate internal structure */
static BackGroundData* allocate_background_data()
{
  BackGroundData* background_data = g_malloc(sizeof(BackGroundData));
  background_data->background_color = NULL; 
  background_data->background_image = NULL; 
  background_data->back_cr = NULL;
  background_data->background_shape = NULL; 
  background_data->background_image = NULL;
  background_data->background_window = NULL;
  return background_data;
}


/* Destroy the background window */
void destroy_background_window()
{
  if (background_data)
    {
      if (background_data->background_shape)
	{
	  g_object_unref(background_data->background_shape);
	}

      if (background_data->background_window != NULL)
	{ 
	  /* destroy brutally the background window */
	  gtk_widget_destroy(background_data->background_window);
	  background_data->background_window = NULL;
	}

      if (background_data->back_cr)
	{
	  cairo_destroy(background_data->back_cr);
	}

      g_free(background_data->background_color);
      /* unref gtkbuilder */
      if (background_data->backgroundWindowGtkBuilder)
	{
	  g_object_unref (background_data->backgroundWindowGtkBuilder);
	}
      g_free(background_data);
    }
  /* quit gtk */
  gtk_main_quit();
}


/* Clear the background */
void clear_background_window()
{
  g_free(background_data->background_color);
  background_data->background_color = NULL;

  g_free(background_data->background_image);
  background_data->background_image = NULL;
  
  /* 
   * @HACK Deny the mouse input to go below the window putting the opacity greater than 0
   * I avoid a complete transparent window because in some operating system this would become
   * transparent to the pointer input also
   *
   */
  gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), BACKGROUND_OPACITY);

  clear_cairo_context(background_data->back_cr);
  
  gint height;
  gint width;
  gdk_drawable_get_size(background_data->background_window->window, &width, &height);

  /* Instantiate a trasparent pixmap to be used as mask */
  background_data->background_shape = gdk_pixmap_new(NULL, width, height, 1); 
  cairo_t* shape_cr = gdk_cairo_create(background_data->background_shape);
  clear_cairo_context(shape_cr); 
  cairo_destroy(shape_cr);

  /* This allows the mouse event to be passed to the window below */
#ifndef _WIN32
  gdk_window_input_shape_combine_mask(background_data->background_window->window,
				      background_data->background_shape,
				      0, 0);
#endif
}


/* Create the background window */
GtkWidget* create_background_window(gchar* backgroundimage)
{
  background_data = allocate_background_data(); 
  
  if (backgroundimage) 
    {
      background_data->background_image = g_strdup_printf("%s", backgroundimage); 
    } 

  /* Initialize the background window */
  background_data->backgroundWindowGtkBuilder = gtk_builder_new();

  GError* error = NULL;
  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(background_data->backgroundWindowGtkBuilder, BACKGROUND_UI_FILE, &error);

  if (error)
    {
      g_warning ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
      return background_data->background_window;
    }  
 
  background_data->background_window = GTK_WIDGET(gtk_builder_get_object(background_data->backgroundWindowGtkBuilder,"backgroundWindow")); 
  gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), 0);

  gtk_widget_set_usize(background_data->background_window, gdk_screen_width(), gdk_screen_height());

  /* connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(background_data->backgroundWindowGtkBuilder, (gpointer) NULL);

  /* This put in fullscreen and generate an exposure */
  gtk_window_fullscreen(GTK_WINDOW(background_data->background_window));

  gtk_widget_show_all(background_data->background_window);

#ifdef _WIN32
  /* in the gtk 2.16.6 used for windows the gtkbuilder property GtkWindow.double-buffered doesn't exist and then I set this by hands */
  gtk_widget_set_double_buffered(background_data->background_window, FALSE);
#endif
  
  return  background_data->background_window;
}


/* Change the background image of ardesia  */
void change_background_image(gchar *name)
{
  if (background_data->background_color)
    {
      g_free(background_data->background_color);
      background_data->background_color = NULL;
    }

  background_data->background_image = name;
  load_file();
}


/* Change the background color of ardesia  */
void change_background_color(gchar* rgba)
{
  if (background_data->background_image)
    {
      g_free(background_data->background_image);
      background_data->background_image = NULL;
    }  

  if (!(background_data->background_color))
    {
      background_data->background_color = g_strdup_printf("%s", rgba);
    }

  load_color();  
}


/* Get the background window */
GtkWidget* get_background_window()
{
  return background_data->background_window;
}


/* Expose event in background window occurs */
G_MODULE_EXPORT gboolean
back_event_expose(GtkWidget *widget, 
		  GdkEventExpose *event, 
		  gpointer user_data)
{
  gint is_fullscreen = gdk_window_get_state(widget->window) & GDK_WINDOW_STATE_FULLSCREEN;
  if (!is_fullscreen)
    {
      return TRUE;
    }
  if (!background_data->back_cr)
    {
      background_data->back_cr = gdk_cairo_create(widget->window); 
    }

  if (background_data->background_image)
    {
      change_background_image(background_data->background_image);
    }
  else if (background_data->background_color)
    {
      change_background_color(background_data->background_color);
    }
  else
    {
      clear_background_window();    
    }

  return TRUE;
}

/* Set the background window */
void set_background_window(GtkWidget* widget)
{
  background_data->background_window = widget;
}
