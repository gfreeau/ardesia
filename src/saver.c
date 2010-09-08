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

#include <saver.h>
#include <utils.h>


/* Save the contents of the pixbuf in the file with name filename */
gboolean save_png (GdkPixbuf *pixbuf,const gchar *filename)
{
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *surface;
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(surface);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
  cairo_paint(cr);
  
  /* write in png */
  cairo_surface_write_to_png(surface, filename);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return TRUE;
}


/* Grab the screenshoot and put it in the GdkPixbuf */
GdkPixbuf* grab_screenshot()
{
  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  return gdk_pixbuf_get_from_drawable (NULL, gdk_get_default_root_window (), NULL,
                                       0, 0, 0, 0, width, height);
}


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 */
void start_save_image_dialog(GtkToolButton *toolbutton, GtkWindow *parent, gchar** workspace_dir)
{

  if (!(*workspace_dir))
    {
      /* Initialize it to the desktop folder */
      gchar* desktop_dir = (gchar *) get_desktop_dir();
      gint lenght = strlen(desktop_dir);
      *workspace_dir = (gchar*) g_malloc( ( lenght + 1) * sizeof(gchar));
      strcpy(*workspace_dir, desktop_dir);
	  
      g_free(desktop_dir);
    }	

  GdkPixbuf* buf = grab_screenshot(); 
  
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save image"), 
 						    parent, 
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  
  gtk_window_set_modal(GTK_WINDOW(chooser), TRUE); 
  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Choose a file")); 
  
  /* preview of saving */
  GtkWidget*   preview = gtk_image_new ();
  gint preview_width = 128;
  gint preview_height = 128;
  GdkPixbuf*   previewPixbuf = gdk_pixbuf_scale_simple(buf, preview_width, preview_height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), previewPixbuf);
  
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(chooser), preview);   
  g_object_unref (previewPixbuf);

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);
 
  gchar* start_string = "ardesia_";
  gchar* date = get_date(); 
  gchar* filename = (gchar*) g_malloc((strlen(start_string) + strlen(date) + 1) * sizeof(gchar));
  sprintf(filename,"%s%s", start_string, date);
  
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      screenshot = TRUE;
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      gchar* supported_extension = ".png";
      gchar* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, supported_extension) != 0))
        {
          filename = (gchar *) realloc(filename,  (strlen(filename) + strlen(supported_extension) + 1) * sizeof(gchar)); 
          (void) strcat((gchar *)filename, supported_extension);
        }           
      if (file_exists(filename,(gchar *) workspace_dir))
        {
	  GtkWidget *msg_dialog; 
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), 
					       GTK_DIALOG_MODAL, 
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_YES_NO, gettext("File Exists. Overwrite"));
	  
          gint result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
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


