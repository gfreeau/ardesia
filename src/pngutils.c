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
#include <pngutils.h>
#include <stdlib.h> 

GtkWidget* background_window = NULL;


/* Save the contents of the pixfuf in the file with name filename */
gboolean save_png (GdkPixbuf *pixbuf,const char *filename)
{

  FILE *handle;
  int width, height, depth, rowstride;
  guchar *pixels;
  png_structp png_ptr;
  png_infop info_ptr;
  png_text text[2];
  int i;

  handle = fopen (filename, "w");
  if (!handle) {
    return FALSE;
  }

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    return FALSE;
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct (&png_ptr, (png_infop *) NULL);
    return FALSE;
  }

  if (setjmp (png_ptr->jmpbuf)) {
    /* Error handler */
    png_destroy_write_struct (&png_ptr, &info_ptr);
    return FALSE;
  }

  png_init_io (png_ptr, handle);
  
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  depth = gdk_pixbuf_get_bits_per_sample (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  gboolean  has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
 
  if (has_alpha) 
  {
    png_set_IHDR (png_ptr, info_ptr, width, height,
 		  depth, PNG_COLOR_TYPE_RGB_ALPHA,
 		  PNG_INTERLACE_NONE,
 		  PNG_COMPRESSION_TYPE_DEFAULT,
 		  PNG_FILTER_TYPE_DEFAULT);
  } 
  else 
   {
     png_set_IHDR (png_ptr, info_ptr, width, height,
		  depth, PNG_COLOR_TYPE_RGB,
		  PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT,
		  PNG_FILTER_TYPE_DEFAULT);
   }

  /* Some text to go with the image such as an header */
  text[0].key = "Title";
  text[0].text = (char *)filename;
  text[0].compression = PNG_TEXT_COMPRESSION_NONE;
  text[1].key = "Software";
  text[1].text = "Ardesia";
  text[1].compression = PNG_TEXT_COMPRESSION_NONE;
  png_set_text (png_ptr, info_ptr, text, 2);

  png_write_info (png_ptr, info_ptr);
  for (i = 0; i < height; i++) {
    png_bytep row_ptr = pixels;

    png_write_row (png_ptr, row_ptr);
    pixels += rowstride;
  }

  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct (&png_ptr, &info_ptr);

  fflush (handle);
  fclose (handle);

  return TRUE;

}


/* Load the contents of the file image with name "filename" into the pixbuf */
gboolean
load_png (const char *filename, GdkPixbuf **pixmap)
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


/* Change the background image of ardesia  */
void change_background_image (const char *name)
{
   
  /* remove old background */
  remove_background();

  background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(background_window), FALSE);
  gtk_widget_set_app_paintable(background_window, TRUE);
  gtk_widget_set_double_buffered(background_window, FALSE);

  
  GdkPixbuf *pixbuf = NULL;
  load_png(name,&pixbuf);
  if (pixbuf==NULL)
  {
      g_printerr("pixbuf is null !\n");
  }

  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen *screen = gdk_display_get_default_screen (display); 
  GdkColormap *colormap;
  colormap = gdk_screen_get_rgba_colormap(screen);
  if (colormap == NULL)
    {
      /* alpha channel is not supported then I try to use the plain rgb */
      colormap = gdk_screen_get_rgb_colormap (screen);
    }
  gtk_widget_set_default_colormap(colormap);

  GtkWidget* darea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(background_window), darea);
  gtk_widget_set_size_request(darea, gdk_pixbuf_get_width(pixbuf),gdk_pixbuf_get_height(pixbuf));
  gtk_widget_realize(darea);
  g_assert(darea->window);
      
  GdkPixmap *pixmap = gdk_pixmap_new(darea->window, gdk_pixbuf_get_width(pixbuf),
				     gdk_pixbuf_get_height(pixbuf), -1);
  g_assert(pixmap);
  gdk_pixbuf_render_to_drawable(pixbuf, pixmap,
				darea->style->fg_gc[GTK_STATE_NORMAL],
				0, 0, 0, 0, -1, -1,
				GDK_RGB_DITHER_NORMAL, 0, 0);

 
  gdk_window_set_back_pixmap(darea->window, pixmap, FALSE);  
  
  gtk_widget_show_all(background_window);
  cairo_t * cr=gdk_cairo_create(darea->window);
  cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0.0, 0.0);
  cairo_fill(cr);
  cairo_paint(cr);
  cairo_destroy(cr);  

    
  gdk_pixbuf_unref(pixbuf);

}


/* Change the background color of ardesia  */
void change_background_color (char *bg_color)
{
  remove_background();

  background_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_fullscreen(GTK_WINDOW(background_window));
  gtk_window_set_decorated(GTK_WINDOW(background_window), FALSE);

  char* rgbcolor= malloc(strlen(bg_color)+2);
  strncpy(&rgbcolor[1],bg_color,8);
  rgbcolor[0]='#';

  GdkColor color;
  
  GdkColormap *colormap;
  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  colormap = gdk_screen_get_rgba_colormap (screen);
  if (colormap == NULL)
    {
      /* alpha channel is not supported then I try to use plain rgb */
      colormap = gdk_screen_get_rgb_colormap (screen);
    }
 
  gtk_widget_push_colormap(colormap);
  gtk_widget_push_visual(gdk_rgba_get_visual());

  gdk_color_parse(rgbcolor, &color);
  gtk_widget_modify_bg(background_window, GTK_STATE_NORMAL, &color);
  gtk_widget_show_all(background_window);
  free(rgbcolor);
  g_free (bg_color);
}


/* Make the screenshot */
void make_screenshot(char* filename)
{

  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  GdkWindow* root = gdk_get_default_root_window ();
  GdkPixbuf* buf = gdk_pixbuf_get_from_drawable (NULL, root, NULL,
						 0, 0, 0, 0, width, height);
  /* store the pixbuf grabbed on file */
  save_png (buf, filename);
  g_object_unref (G_OBJECT (buf));

}


/* Remove the background */
void remove_background()
{
  if (background_window!=NULL)
    { 
      /* destroy brutally the background window */
      gtk_widget_destroy(background_window);
    }
}

