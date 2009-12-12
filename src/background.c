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
 
/* This is the file whith the code to handle images */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <png.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <background.h>
#include <annotate.h>
#include <stdlib.h> 


typedef struct
{
  gchar*     rgb;
  gchar*     a;
} BackgroundColorData;


GtkWidget* background_window = NULL;


/* Load the contents of the file image with name "filename" into the pixbuf */
gboolean load_png (const char *filename, GdkPixbuf **pixmap)
{
  *pixmap = gdk_pixbuf_new_from_file (filename, NULL);

  if (*pixmap)
    {
      GdkPixbuf *scaled;
      gint height = gdk_screen_height ();
      gint width = gdk_screen_width ();
      scaled = gdk_pixbuf_scale_simple(*pixmap, width, height, GDK_INTERP_BILINEAR);
      g_object_unref (G_OBJECT (*pixmap));
      *pixmap = scaled;
      return TRUE;
    }
  else
    {
       fprintf (stderr, "couldn't load %s\n", filename);
       exit(0);
    }

  *pixmap = NULL;
  return FALSE;
}


static gboolean on_window_file_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  GdkPixbuf *pixbuf = (GdkPixbuf *) data; 
  cairo_t *cr = gdk_cairo_create(widget->window);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0.0, 0.0);

  cairo_paint(cr);
  cairo_destroy(cr);    
  g_object_unref (G_OBJECT (pixbuf));
  
  return TRUE;
}


/* Remove the background */
void clear_background()
{
  if (background_window != NULL)
    { 
      /* destroy brutally the background window */
      gtk_widget_destroy(background_window);
    }
}


static gboolean on_window_color_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  BackgroundColorData* bg_data = (BackgroundColorData*) data;
  cairo_t *cr = gdk_cairo_create(widget->window);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  int r,g,b,a;
  sscanf (bg_data->rgb, "%02X%02X%02X", &r, &g, &b);
  sscanf (bg_data->a, "%02X", &a);
  cairo_set_source_rgba (cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);

  cairo_paint(cr);
  cairo_destroy(cr);    
  free(bg_data);
  return TRUE;
}


/* Create the background window */
void create_background_window()
{
  background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_fullscreen(GTK_WINDOW(background_window));
  gtk_window_stick(GTK_WINDOW(background_window));  
  gtk_window_set_decorated(GTK_WINDOW(background_window), FALSE);
  gtk_widget_set_app_paintable(background_window, TRUE);
  gtk_window_set_opacity(GTK_WINDOW(background_window), 1); 
  gtk_widget_set_double_buffered(background_window, FALSE);
}


/* Change the background image of ardesia  */
void change_background_image (const char *name)
{
  clear_background();
  GdkPixbuf *pixbuf = NULL; 
  load_png(name,&pixbuf);   
  
  create_background_window();

  g_signal_connect(G_OBJECT(background_window), "expose-event", G_CALLBACK(on_window_file_expose_event), pixbuf);

  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  
  GdkColormap *colormap = gdk_screen_get_rgba_colormap (screen);
  gtk_widget_set_default_colormap(colormap);

  gtk_widget_show_all(background_window);
}


/* Change the background color of ardesia  */
void change_background_color (char* rgb, char *a)
{
  clear_background();
  
  BackgroundColorData *data = malloc (sizeof (BackgroundColorData));
  data->rgb = rgb; 
  data->a = a; 

  create_background_window();
  
  g_signal_connect(G_OBJECT(background_window), "expose-event", G_CALLBACK(on_window_color_expose_event), data);

  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  
  GdkColormap *colormap = gdk_screen_get_rgba_colormap (screen);
  gtk_widget_set_default_colormap(colormap);

  gtk_widget_show_all(background_window);
}
