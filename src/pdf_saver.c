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

#include <annotation_window.h>
#include <pdf_saver.h>
#include <utils.h>
#include <saver.h>
#include <keyboard.h>


/* internal structure allocated once. */
static PdfData *pdf_data;


/* Start the dialog that ask the file name where is being exported the pdf. */
static gboolean
start_save_pdf_dialog (GtkWindow *parent,
                       GdkPixbuf *pixbuf)
{
  gboolean ret = TRUE;
  GtkWidget *preview = NULL;
  gint preview_width = 128;
  gint preview_height = 128;
  GdkPixbuf *preview_pixbuf = NULL;
  gchar *filename = "";
  gchar *supported_extension = ".pdf";

  gdk_window_set_cursor (gtk_widget_get_window(get_annotation_window ()), (GdkCursor *) NULL);
  
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

  /* Save the preview in a image buffer. */
  preview = gtk_image_new ();
  preview_pixbuf = gdk_pixbuf_scale_simple (pixbuf, preview_width, preview_height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), preview_pixbuf);
  
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
  g_object_unref (preview_pixbuf);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), get_project_dir ());

  filename = get_default_filename ();

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), filename);

  start_virtual_keyboard ();

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      g_free (filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      if (!g_str_has_suffix (filename, supported_extension))
        {
          pdf_data->filename = g_strdup_printf ("%s%s",filename,supported_extension);
        }
      else
        {
          pdf_data->filename = g_strdup_printf ("%s",filename);
        }

      g_free (filename);

      if (file_exists (pdf_data->filename))
        {
          gint result = show_override_dialog (GTK_WINDOW (chooser));
          if ( result == GTK_RESPONSE_NO)
            {
              ret = FALSE;
            }
        }
    }

  stop_virtual_keyboard ();
  gtk_widget_destroy (preview);
  if (chooser != NULL)
    {
      gtk_widget_destroy (chooser);
      chooser = NULL;
    }
  return ret;
}


/* Initialize the pdf saver. */
static gboolean 
init_pdf_saver (GtkWindow *parent,
                GdkPixbuf *pixbuf)
{
  gboolean ret = FALSE;

  pdf_data = (PdfData *) g_malloc ( (gsize) sizeof (PdfData));
  pdf_data->thread = NULL;
  pdf_data->input_filelist = NULL;
  pdf_data->filename = NULL;

  /* Start the widget to ask the file name where save the pdf. */
  ret = start_save_pdf_dialog (parent, pixbuf);

  if (!ret)
    {
      return FALSE;
    }

  /* Add to the list of the artefacts created in the session. */
  add_artifact (pdf_data->filename);

  return TRUE;
}


/* Save the surfaces in the pdf file. */
static void
pdf_save ()
{
  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  /* create the cairo surface for pdf */
  cairo_surface_t *pdf_surface = cairo_pdf_surface_create (pdf_data->filename, width, height);
  cairo_t *pdf_cr = cairo_create (pdf_surface);

  gint lenght = g_slist_length (pdf_data->input_filelist);
  
  gint i;

  for (i=lenght-1; i>=0; i--)
    {
      gchar *current_filename = (gchar *) g_slist_nth_data (pdf_data->input_filelist, i);
      /* load the file name content */
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (current_filename, NULL);
      gdk_cairo_set_source_pixbuf (pdf_cr, pixbuf, 0, 0);
      cairo_paint (pdf_cr);
      cairo_show_page (pdf_cr);
      g_object_unref (pixbuf);
    }

  cairo_surface_flush (pdf_surface);
  /* destroy */
  cairo_surface_destroy (pdf_surface);
  cairo_destroy (pdf_cr);
}


/* Wait if there is a pending thread. */
static
void wait_for_pdf_save_pending_thread ()
{
  if (pdf_data->thread)
    {
      g_thread_join (pdf_data->thread);
      pdf_data->thread = NULL;
    }
}


/* Add the screenshot to pdf. */
void
add_pdf_page (GtkWindow *parent)
{
  GdkPixbuf *pixbuf = grab_screenshot ();
  cairo_surface_t *saved_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                               gdk_screen_width (),
                                                               gdk_screen_height ());

  cairo_t *cr = cairo_create (saved_surface);
  const gchar *tmp_dir = g_get_tmp_dir ();
  gchar *default_filename = get_default_filename ();
  gchar *screenshoot_name = g_strdup_printf ("%s_screenshoot.png", default_filename);
  gchar *filename  = g_build_filename (tmp_dir, screenshoot_name, (gchar *) 0);
  GError *err = NULL ;

  g_free (screenshoot_name);
  
  g_free (default_filename);

  /* Load a surface with the data->annotation_cairo_context content and write the file. */
  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);
  cairo_surface_write_to_png (saved_surface, filename);
  cairo_surface_destroy (saved_surface); 
  cairo_destroy (cr);

  if (pdf_data == NULL)
    {
      if (!g_thread_supported ())
        {
          /* Initialize internal mutex "gdk_threads_mutex". */
          g_thread_init (NULL);
          gdk_threads_init ();
          g_printerr ("g_thread supported\n");
        }
      if (!init_pdf_saver (parent, pixbuf))
        {
          g_object_unref (pixbuf);
          return;
        }
    }

  g_object_unref (pixbuf);
  pdf_data->input_filelist = g_slist_prepend (pdf_data->input_filelist, filename);

  wait_for_pdf_save_pending_thread ();

  /* Start save thread. */
  if ( (pdf_data->thread = g_thread_create ( (GThreadFunc) pdf_save, (void *) NULL, TRUE, &err)) == NULL)
    {
      g_printerr ("Thread create failed: %s!!\n", err->message );
      g_error_free (err) ;
    }

}


/* Quit the pdf saver. */
void
quit_pdf_saver ()
{
  if (pdf_data)
    {
      wait_for_pdf_save_pending_thread ();

      /* Free the list and all the buffers inside it. */
      while (pdf_data->input_filelist)
        {
          gchar *filename = (gchar *) g_slist_nth_data (pdf_data->input_filelist, 0);
          if (filename)
            {
              g_remove (filename);
              pdf_data->input_filelist = g_slist_remove (pdf_data->input_filelist, filename);
              g_free (filename);
              filename = NULL;
            }
        }

      if (pdf_data->filename)
        {
          g_free (pdf_data->filename);
          pdf_data->filename = NULL;
        }

      g_free (pdf_data);
      pdf_data = NULL;
    }
}


