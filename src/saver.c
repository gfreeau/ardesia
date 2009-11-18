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


#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include "pngutils.h"
#include "utils.h"


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

  
  GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save image as image", parent, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  
  /* with this apperar in all the workspaces */  
  gtk_window_stick((GtkWindow*) chooser);
 
  gtk_window_set_title (GTK_WINDOW (chooser), "Select a file");
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), workspace_dir);
  
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

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
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,  GTK_BUTTONS_YES_NO, "File Exists. Overwrite");
          gtk_window_stick((GtkWindow*)msg_dialog);

          int result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog != NULL)
            { 
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result == GTK_RESPONSE_NO)
            { 
	      g_free(filename);
	      return; 
	    } 
	}
      screenshot = TRUE;
    }
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
  g_object_unref (G_OBJECT (buf));
}
