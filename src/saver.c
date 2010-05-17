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

#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include "utils.h"

#include <cairo.h>

#if defined(_WIN32)
  #include <cairo-win32.h>
  #include <gdkwin32.h>
#else
  #include <cairo-xlib.h>
#endif

/* Save the contents of the pixbuf in the file with name filename */
gboolean save_png (GdkPixbuf *pixbuf,const char *filename)
{
  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *surface;
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(surface);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
  cairo_paint(cr);
  cairo_surface_write_to_png(surface, filename);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return TRUE;
}


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 */
void start_save_image_dialog(GtkToolButton   *toolbutton, GtkWindow *parent, gchar** workspace_dir)
{
  char* date = get_date();
  if (!(*workspace_dir))
    {
      /* Initialize it to the desktop folder */
      gchar* desktop_dir = (gchar *) get_desktop_dir();
      *workspace_dir = g_malloc( strlen(desktop_dir)*sizeof(gchar));
      strcpy(*workspace_dir, desktop_dir);
      g_free(desktop_dir);
    }	

  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  GdkPixbuf* buf = gdk_pixbuf_get_from_drawable (NULL, gdk_get_default_root_window (), NULL,
                                                 0, 0, 0, 0, width, height);

  
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save image"), 
 						    parent, 
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  
  gtk_window_set_modal(GTK_WINDOW(chooser), TRUE); 

  /* preview of saving */
  GtkWidget*   preview = gtk_image_new ();
  GdkPixbuf*   previewPixbuf = gdk_pixbuf_scale_simple(buf, 128, 128, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), previewPixbuf);
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(chooser), preview);   
  g_object_unref (previewPixbuf);

  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Select a file"));
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);
  
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      screenshot = TRUE;
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      char* tmp = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
      strcpy(*workspace_dir, tmp);
      free(tmp);
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
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), 
					       GTK_DIALOG_MODAL, 
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_YES_NO, gettext("File Exists. Overwrite"));
          
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
  g_free(date);
  g_free(filename);
  g_object_unref (buf);
}
