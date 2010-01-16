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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include "utils.h"
#include "gettext.h"

#include <png.h>

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
  if (!handle) 
    {
      return FALSE;
    }

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL)
    {
      return FALSE;
    }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) 
    {
      png_destroy_write_struct (&png_ptr, (png_infop *) NULL);
      return FALSE;
    }

  if (setjmp (png_ptr->jmpbuf))
    {
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
  for (i = 0; i < height; i++)
    {
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


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 */
void start_save_image_dialog(GtkToolButton   *toolbutton, GtkWindow *parent, char* workspace_dir)
{
  char * date = get_date();
  if (workspace_dir==NULL)
    {
      workspace_dir = (char *) get_desktop_dir();
    }	

  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  GdkWindow* root = gdk_get_default_root_window ();
  GdkPixbuf* buf = gdk_pixbuf_get_from_drawable (NULL, root, NULL,
                                                 0, 0, 0, 0, width, height);

  
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save image"), parent, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  
  /* with this apperar in all the workspaces */  
  gtk_window_stick((GtkWindow*) chooser);
 
  /* preview of saving */
  GtkWidget*   preview = gtk_image_new ();
  GdkPixbuf*   previewPixbuf = gdk_pixbuf_scale_simple(buf, 128, 128, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), previewPixbuf);
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(chooser), preview);   
  g_object_unref (previewPixbuf);

 
  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Select a file"));
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), workspace_dir);
  
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      screenshot = TRUE;
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
      char* supported_extension = ".png";
      char* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, supported_extension) != 0))
        {
          filename = (gchar *) realloc(filename,  (strlen(filename) + strlen(supported_extension) + 1) * sizeof(gchar)); 
          (void) strcat((gchar *)filename, supported_extension);
          free(extension);
        }           

      if (file_exists(filename,(char *) workspace_dir))
        {
	  GtkWidget *msg_dialog; 
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,  GTK_BUTTONS_YES_NO, gettext("File Exists. Overwrite"));
          gtk_window_stick((GtkWindow*)msg_dialog);

          int result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog != NULL)
            { 
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result == GTK_RESPONSE_NO)
            {
		screenshot = FALSE;
	    } 
	}
    }
  gtk_widget_destroy (preview);
  if (chooser != NULL)
    {
      gtk_widget_destroy (chooser);
    }
  if (screenshot)
    {
      /* store the pixbuf grabbed on file */
      save_png (buf, filename);
    }
  free(date);
  g_free(filename);
  g_object_unref (buf);
}
