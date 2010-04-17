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

GtkWidget* background_window = NULL;
char* background_color = NULL;
GdkPixbuf *pixbuf = NULL; 


/* Load the contents of the file image with name "filename" into the pixbuf */
void load_png (const char *filename)
{
  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  GdkPixbuf *scaled = NULL;

  if (pixbuf)
    {
      gint height = gdk_screen_height ();
      gint width = gdk_screen_width();
      scaled = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
      g_object_unref(G_OBJECT (pixbuf));
    }
  else
    {
      fprintf (stderr, "couldn't load %s\n", filename);
      exit(0);
    }
  pixbuf = scaled;
}

/* Transparent rectangle to allow to see the bar and pass the event below */
void make_hole(GtkWidget *widget, cairo_t * cr)
{
  int x,y, width, height;
  GtkWindow* bar = GTK_WINDOW(get_bar_window());
  gtk_window_get_position(bar, &x, &y);
  gtk_window_get_size(bar, &width, &height);    

  GdkPixmap* shape = gdk_pixmap_new (NULL, width, height, 1);

  cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_rectangle(cr, x, y, width, height);
  cairo_fill(cr);	

  cairo_t *shape_cr = gdk_cairo_create(shape);
  cairo_set_operator(shape_cr,CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (shape_cr, 1, 1, 1, 1);
  cairo_paint(shape_cr);
  if (shape_cr)
    {
      cairo_set_operator(shape_cr,CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (shape_cr, 0, 0, 0, 0);
      cairo_paint(shape_cr);
      cairo_fill(shape_cr);	
    }	
  gtk_widget_input_shape_combine_mask(widget, shape, 0, 0);
  cairo_destroy(shape_cr);
   
}

/* The windows has been exposed after the show_all request to change the background image */
G_MODULE_EXPORT gboolean
on_window_file_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *back_cr = gdk_cairo_create(widget->window);
  cairo_set_operator(back_cr, CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(back_cr, pixbuf, 0.0, 0.0);
  cairo_paint(back_cr);
  make_hole(widget, back_cr);
  cairo_destroy(back_cr);     
  return FALSE;
}


/* Remove the background */
void clear_background(GtkWidget* window)
{
  if (background_window != NULL)
    { 
      /* destroy brutally the background window */
      gtk_widget_destroy(background_window);
      background_window = NULL;
      if (pixbuf)
	{
	  g_object_unref(G_OBJECT (pixbuf));
	  pixbuf = NULL;
	}
    }
}


/* The windows has been exposed after the show_all request to change the background color */
G_MODULE_EXPORT gboolean
on_window_color_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr = gdk_cairo_create(widget->window);
  if (cr)
    {
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      int r,g,b,a;
      sscanf(background_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
      cairo_set_source_rgba(cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);
      cairo_paint(cr);
      make_hole(widget, cr);
      cairo_destroy(cr);
    }  
  return TRUE;
}


/* Create the background window */
void create_background_window(GtkWidget* widget)
{
  background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_transient_for(GTK_WINDOW(get_annotation_window()), GTK_WINDOW(background_window));
  //gtk_window_set_modal(GTK_WINDOW(background_window), TRUE);

  gtk_widget_set_usize (GTK_WIDGET(background_window), gdk_screen_width(), gdk_screen_height());
  gtk_window_fullscreen(GTK_WINDOW(background_window));
  if (DOCK)
   {
     gtk_window_set_type_hint(GTK_WINDOW(background_window), GDK_WINDOW_TYPE_HINT_DOCK); 
   }
  if (STICK)
    {
      gtk_window_stick(GTK_WINDOW(background_window));  
    }
  
  gtk_window_set_decorated(GTK_WINDOW(background_window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(background_window), TRUE);

  gtk_widget_set_app_paintable(background_window, TRUE);
  
  #ifndef _WIN32 
    gtk_window_set_opacity(GTK_WINDOW(background_window), 1); 
  #else
    // TODO In windows I am not able to use an rgba transwidget  
    // windows and then I use a sort of semi transparency
    gtk_window_set_opacity(GTK_WINDOW(background_window), 0.5); 
  #endif 

  gtk_widget_show_all(background_window);
}


/* Change the background image of ardesia  */
void change_background_image (GtkWidget* window, const char *name)
{
  clear_background(window);
  load_png(name);   
  create_background_window(window);

  g_signal_connect(G_OBJECT(background_window), "expose-event", G_CALLBACK(on_window_file_expose_event), NULL);

  gtk_widget_show(background_window);
}


/* Change the background color of ardesia  */
void change_background_color (GtkWidget* window, char* rgba)
{
  if (background_color == NULL)
    {
      background_color=g_malloc(9);
    }
  
  strcpy(background_color, rgba);
  
  clear_background(window);
 
  create_background_window(window);
  
  g_signal_connect(G_OBJECT(background_window), "expose-event", G_CALLBACK(on_window_color_expose_event), NULL);

  
  gtk_widget_set_app_paintable(background_window, TRUE);
  gtk_widget_set_double_buffered(background_window, FALSE);
}
