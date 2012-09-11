/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
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
#  include <config.h>
#endif

#include <saver.h>
#include <utils.h>
#include <keyboard.h>


/* Confirm to override file dialog. */
gboolean
show_override_dialog (GtkWindow *parent)
{
  GtkWidget *msg_dialog = (GtkWidget *) NULL;
  gint result = GTK_RESPONSE_NO;
  
  msg_dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_WARNING,
                                       GTK_BUTTONS_YES_NO,
                                       gettext ("File Exists. Overwrite"));

  //gtk_window_set_keep_above (GTK_WINDOW (msg_dialog), TRUE);

  result = gtk_dialog_run (GTK_DIALOG (msg_dialog));
    if (msg_dialog)
      {
        gtk_widget_destroy (msg_dialog);
        msg_dialog = NULL;
      }

   return result;
}


/* Show the could not write the file */
void
show_could_not_write_dialog (GtkWindow *parent_window)
{
  GtkWidget *permission_denied_dialog = (GtkWidget *) NULL;
  
  permission_denied_dialog = gtk_message_dialog_new (parent_window,
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     gettext ("Couldn't open file for writing: Permission denied"));
	
  gtk_window_set_modal (GTK_WINDOW (permission_denied_dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (permission_denied_dialog));
    if (permission_denied_dialog)
      {
        gtk_widget_destroy (permission_denied_dialog);
        permission_denied_dialog = NULL;
      }
}


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot.
 */
void
start_save_image_dialog (GtkToolButton *toolbutton,
                         GtkWindow     *parent)
{

  GtkWidget *preview = NULL;
  gint preview_width = 128;
  gint preview_height = 128;
  GdkPixbuf *preview_pixbuf = NULL;
  gchar  *filename = "";
  gchar *filename_copy = "";
  gchar *supported_extension = ".pdf";
  gint run_status = GTK_RESPONSE_NO;
  gboolean screenshot = FALSE;
  GdkPixbuf *buf = grab_screenshot ();

  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext ("Export as pdf"),
                                                    parent,
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_SAVE_AS,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);

  gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (chooser), TRUE);

  gtk_window_set_title (GTK_WINDOW (chooser), gettext ("Choose a file"));

  /* Save the preview in a buffer. */
  preview = gtk_image_new ();
  preview_pixbuf = gdk_pixbuf_scale_simple (buf, preview_width, preview_height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), preview_pixbuf);
  
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
  g_object_unref (preview_pixbuf);
  preview_pixbuf = NULL;

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), get_project_dir ());

  filename = get_default_filename ();

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), filename);

  start_virtual_keyboard ();
  run_status = gtk_dialog_run (GTK_DIALOG (chooser));
  if (run_status == GTK_RESPONSE_ACCEPT)
    {
      g_free (filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      filename_copy = g_strdup_printf ("%s", filename);

      screenshot = TRUE;
      supported_extension = ".png";

      if (!g_str_has_suffix (filename, supported_extension))
        {
          g_free (filename_copy);
          filename_copy = g_strdup_printf ("%s%s", filename, supported_extension);
        }

      g_free (filename);
      filename = filename_copy;

      if (file_exists (filename))
        {
          gint result = show_override_dialog (GTK_WINDOW (chooser));
          if ( result == GTK_RESPONSE_NO)
            {
              screenshot = FALSE;
            } 
        }
      else
        {
           FILE *stream = g_fopen (filename, "w");
           if (stream == NULL)
            {
              show_could_not_write_dialog (GTK_WINDOW (chooser));
            }
           else
            {
              fclose (stream);
            }
        }
    }
  stop_virtual_keyboard ();
  gtk_widget_destroy (preview);
  preview = NULL;
  if (chooser != NULL)
    {
      gtk_widget_destroy (chooser);
      chooser = NULL;
    }
  if (screenshot)
    {
      /* Store the buffer on file. */
      save_pixbuf_on_png_file (buf, filename);
      /* Add to the list of the artefacts created in the session. */
      add_artifact (filename);
    }

  g_free (filename);
  filename = NULL;
  g_object_unref (buf);
  buf = NULL;
}


