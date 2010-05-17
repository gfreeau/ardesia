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
 
/* This is the file whith the code to handle images */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <background.h>
#include <annotate.h>
#include <stdlib.h> 
#include <utils.h> 

#include <cairo.h>

#if defined(_WIN32)
	#include <cairo-win32.h>
#else
	#include <cairo-xlib.h>
#endif

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

static BackGroundData* background_data;

/* Load the contents of the file image with name "filename" into the pixbuf */
GdkPixbuf * load_png (char *filename)
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
      fprintf (stderr, "couldn't load the file %s as background\n", filename);
      exit(0);
    }
  pixbuf = scaled;
  return pixbuf;
}


/* Destroy the background window */
void destroy_background_window()
{
  if (background_data->background_shape)
   {
      g_object_unref(background_data->background_shape);
   }
  if (background_data->background_window != NULL)
   { 
      /* destroy brutally the background window */
      gtk_widget_destroy(background_data->background_window);
   }
  if (background_data->back_cr)
    {
      cairo_destroy(background_data->back_cr);
    }
  g_free(background_data->background_color);
  g_free(background_data);
}


/* Clear the background */
void clear_background_window()
{
  g_free(background_data->background_color);
  background_data->background_color = NULL;
  g_free(background_data->background_image);
  background_data->background_image = NULL;
  #ifdef _WIN32
    gdk_window_shape_combine_mask (background_data->background_window->window,  background_data->background_shape, 0, 0);
  #endif
  cairo_set_operator(background_data->back_cr, CAIRO_OPERATOR_CLEAR);
  gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), 0);
  cairo_paint(background_data->back_cr);
  cairo_stroke(background_data->back_cr); 
  
  gint height;
  gint width;
  gdk_drawable_get_size(background_data->background_window->window, &width, &height);


  /* Make the trasparen pixmap to be used as mask */
  background_data->background_shape = gdk_pixmap_new (NULL, width, height, 1); 
  cairo_t* shape_cr = gdk_cairo_create(background_data->background_shape);
  clear_cairo_context(shape_cr); 
  cairo_destroy(shape_cr);

  /* This allows the mouse event to be passed to the window below */
  #ifdef _WIN32
    gdk_window_shape_combine_mask (background_data->background_window->window,  
                                   background_data->background_shape,
                                   0, 0);
  #else
    gdk_window_input_shape_combine_mask (background_data->background_window->window,
                                         background_data->background_shape,
                                         0, 0);
  #endif
}


G_MODULE_EXPORT gboolean
back_event_expose (GtkWidget *widget, 
              GdkEventExpose *event, 
              gpointer user_data)
{
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
       if (background_data->background_color)
         {
            change_background_color(background_data->background_color);
         }
    }
  else
    {
       clear_background_window();    
    }
  return TRUE;
}


/* The windows has been exposed after the show_all request to change the background image */
void load_file()
{
    if (background_data->back_cr)
    {
      #ifdef _WIN32
        gdk_window_shape_combine_mask (background_data->background_window->window,
                                       NULL, 
                                       0, 0);
      #endif
      GdkPixbuf* pixbuf = load_png(background_data->background_image);   
      cairo_set_operator(background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), 1);
      gdk_cairo_set_source_pixbuf(background_data->back_cr, pixbuf, 0.0, 0.0);
      cairo_paint(background_data->back_cr);
      cairo_stroke(background_data->back_cr);   
      g_object_unref(G_OBJECT (pixbuf));
      #ifdef _WIN32
        gdk_window_shape_combine_mask (background_data->background_window->window,
                                       NULL,
                                       0, 0);
      #else 
        gdk_window_input_shape_combine_mask (background_data->background_window->window,
                                             NULL, 
                                             0, 0);
      #endif
    }
}


/* The windows has been exposed after the show_all request to change the background color */
void load_color()
{
  if (background_data->back_cr)
    {
      #ifdef _WIN32
        gdk_window_shape_combine_mask (background_data->background_window->window,
                                       NULL, 
                                       0, 0);
      #endif
      cairo_set_operator(background_data->back_cr, CAIRO_OPERATOR_SOURCE);
      int r,g,b,a;
      sscanf(background_data->background_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
      gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), (double) a/256);
      cairo_set_source_rgb(background_data->back_cr, (double) r/256, (double) g/256, (double) b/256);
      cairo_paint(background_data->back_cr);
      cairo_stroke(background_data->back_cr);
      #ifdef _WIN32
        gdk_window_shape_combine_mask (background_data->background_window->window,
                                       NULL,
                                       0, 0);
      #else 
         /* This deny the mouse event to be passed to the window below */
        gdk_window_input_shape_combine_mask (background_data->background_window->window,  
                                             NULL, 
                                             0, 0);
      #endif
    }  
}


/* Change the background image of ardesia  */
void change_background_image (char *name)
{
   g_free(background_data->background_color);
   background_data->background_color = NULL;

   background_data->background_image = name;
   load_file();
}


/* Change the background color of ardesia  */
void change_background_color (char* rgba)
{
  g_free(background_data->background_image);
  background_data->background_image = NULL;
  if (!(background_data->background_color))
    {
       background_data->background_color = (char*) g_malloc( sizeof(char) * 9);
    }
 
  strcpy(background_data->background_color, rgba);
  load_color();  
}


/* Get the background window */
GtkWidget* get_background_window()
{
  return background_data->background_window;
}


/* Set the background window */
void set_background_window(GtkWidget* widget)
{
  background_data->background_window = widget;
}


/* Create the background window */
GtkWidget* create_background_window(char* backgroundimage)
{
  background_data = g_malloc(sizeof(BackGroundData));
  background_data->background_color = NULL; 
  background_data->background_image = NULL; 
  background_data->back_cr = NULL;
  background_data->background_shape = NULL; 

  background_data->background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(background_data->background_window), "Ardesia");

  gtk_window_set_decorated(GTK_WINDOW(background_data->background_window), FALSE);
  
  gtk_window_maximize(GTK_WINDOW(background_data->background_window));

  gtk_window_set_opacity(GTK_WINDOW(background_data->background_window), 0); 

  g_signal_connect (background_data->background_window, "expose_event",
		    G_CALLBACK (back_event_expose), NULL);

  gtk_widget_show_all(background_data->background_window);
  
  gtk_widget_set_app_paintable(background_data->background_window, TRUE);

  gtk_widget_set_double_buffered(background_data->background_window, FALSE);

  return  background_data->background_window;
}

