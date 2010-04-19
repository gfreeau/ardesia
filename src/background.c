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

char* background_color = NULL;
GdkPixbuf *pixbuf = NULL; 
cairo_t *back_cr = NULL;
GtkWidget* background_window = NULL;


void destroy_pixbuf()
{
  if (pixbuf)
    {
       g_object_unref(G_OBJECT (pixbuf));
       pixbuf = NULL;
    }
}

/* Load the contents of the file image with name "filename" into the pixbuf */
void load_png (const char *filename)
{
  destroy_pixbuf();
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


/* Destroy the background window */
void destroy_background_window()
{
  if (background_window != NULL)
   { 
      /* destroy brutally the background window */
      gtk_widget_destroy(background_window);
      set_background_window(NULL);
   }
  if (pixbuf)
   {
      g_object_unref(G_OBJECT (pixbuf));
      pixbuf = NULL;
   }
  if (back_cr)
    {
      cairo_destroy(back_cr);
    }
}


/* Clear the background */
void clear_background_window()
{
  change_background_color ("00000000");
}


G_MODULE_EXPORT gboolean
back_event_expose (GtkWidget *widget, 
              GdkEventExpose *event, 
              gpointer user_data)
{
  if (!back_cr)
  {
    back_cr = gdk_cairo_create(widget->window);
    clear_background_window();
  }
  return TRUE;
}


/* The windows has been exposed after the show_all request to change the background image */
void load_file()
{
  cairo_set_operator(back_cr, CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(back_cr, pixbuf, 0.0, 0.0);
  cairo_paint(back_cr);
  cairo_stroke(back_cr);   
}


/* The windows has been exposed after the show_all request to change the background color */
void load_color()
{
  if (back_cr)
    {
      cairo_set_operator(back_cr, CAIRO_OPERATOR_SOURCE);
      int r,g,b,a;
      sscanf(background_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
      cairo_set_source_rgba(back_cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);
      cairo_paint(back_cr);
      cairo_stroke(back_cr); 
    }  
}


/* Change the background image of ardesia  */
void change_background_image (const char *name)
{
   load_png(name);   
   load_file();
}


/* Change the background color of ardesia  */
void change_background_color (char* rgba)
{
  background_color = rgba;
  load_color();  
}


/* Get the background window */
GtkWidget* get_background_window()
{
  return background_window;
}


/* Set the background window */
void set_background_window(GtkWidget* widget)
{
  background_window=widget;
}


/* Create the background window */
GtkWidget* create_background_window()
{
  GtkWidget *background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
  
  #ifndef _WIN32 
    gtk_window_set_opacity(GTK_WINDOW(background_window), 1); 
  #else
    // TODO In windows I am not able to use an rgba transwidget  
    // windows and then I use a sort of semi transparency
    gtk_window_set_opacity(GTK_WINDOW(background_window), 0.5); 
  #endif 
  g_signal_connect (background_window, "expose_event",
		    G_CALLBACK (back_event_expose), NULL);
  gtk_widget_set_app_paintable(background_window, TRUE);
  gtk_widget_set_double_buffered(background_window, FALSE);
  gtk_widget_show_all(background_window);
  return  background_window;
}


