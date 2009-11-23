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

#include <png.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <background.h>
#include <annotate.h>
#include <stdlib.h> 

cairo_t* cr = NULL;
cairo_t* back_cr = NULL;


/* Load the contents of the file image with name "filename" into the pixbuf */
gboolean load_png (const char *filename, GdkPixbuf **pixmap)
{
  *pixmap = gdk_pixbuf_new_from_file (filename, NULL);

  if (*pixmap)
    {
      GdkPixbuf *scaled;
      gint height = gdk_screen_height ();
      gint weight = gdk_screen_width ();
      scaled = gdk_pixbuf_scale_simple(*pixmap, weight, height, GDK_INTERP_BILINEAR);
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

/* Create new background */
void create_new_background()
{
  gint width = -1;
  gint height = -1;
  GtkWindow* annotation_window = get_annotation_window();
  gtk_window_get_size ( annotation_window , &width, &height);
  cairo_surface_t *background_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cr = get_annotation_cairo_context();
  
  cairo_set_source_surface (cr, background_surface, 0, 0);
  back_cr = cairo_create(background_surface);
}


/* Clear the background */
void clear_background()
{
  if (back_cr == NULL)
    {
      create_new_background();
    }
   cairo_set_operator(back_cr,CAIRO_OPERATOR_SOURCE);
   cairo_set_source_rgba(back_cr,0,0,0,0);
   cairo_fill(back_cr);
   cairo_paint(back_cr);

   cairo_paint(cr);
}


/* Change the background image of ardesia  */
void change_background_image (const char *name)
{
  if (back_cr == NULL)
    {
      create_new_background();
    }
  clear_background();

  cairo_set_operator(back_cr, CAIRO_OPERATOR_SOURCE);
  GdkPixbuf *pixbuf = NULL; 
  load_png(name,&pixbuf);   
  gdk_cairo_set_source_pixbuf(back_cr, pixbuf, 0.0, 0.0);

  cairo_fill(back_cr);
  cairo_paint(back_cr);

  cairo_paint(cr);
}


/* Change the background color of ardesia  */
void change_background_color (char* rgb, char *alpha)
{
  if (back_cr == NULL)
    {
      create_new_background();
    }
  clear_background();
  cairo_set_operator(back_cr, CAIRO_OPERATOR_SOURCE);

  int r,g,b,a;
  sscanf (rgb, "%02X%02X%02X", &r, &g, &b);
  sscanf (alpha, "%02X", &a);
  cairo_set_source_rgba (back_cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);

  cairo_paint(back_cr);

  cairo_paint(cr);
}



