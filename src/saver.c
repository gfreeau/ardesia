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


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 */
void start_save_image_dialog(GtkToolButton *toolbutton, GtkWindow *parent, gchar** workspace_dir)
{

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
  GtkWidget*   preview = gtk_image_new();
  gint preview_width = 128;
  gint preview_height = 128;
  GdkPixbuf*   previewPixbuf = gdk_pixbuf_scale_simple(buf, preview_width, preview_height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), previewPixbuf);
  
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(chooser), preview);   
  g_object_unref (previewPixbuf);

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);

  gchar* filename = get_default_name();
  
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {   
      /* store the folder location this will be proposed the next time */
      g_free(*workspace_dir);
      *workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser)); 

      g_free(filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      gchar* filenamecopy = g_strdup_printf("%s",filename); 

      screenshot = TRUE;
      gchar* supported_extension = ".png";
     
      if (!g_str_has_suffix(filename, supported_extension))
        {
          g_free(filenamecopy);
          filenamecopy = g_strdup_printf("%s%s",filename,supported_extension);
        }      
      g_free(filename);   
      filename = filenamecopy;  

      if (file_exists(filename))
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
  g_free(filename);
  g_object_unref (buf);
}


