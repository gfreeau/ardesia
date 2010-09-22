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

#include <pdf_saver.h>
#include <utils.h>
#include <saver.h>


static PdfData *pdf_data;


/* Start the dialog that ask the filename where is being exported the pdf */
gboolean start_save_pdf_dialog(GtkWindow *parent, gchar** workspace_dir, GdkPixbuf *pixbuf)
{
   gboolean ret = TRUE;
   
   GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Export as pdf"), 
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
   GdkPixbuf*   previewPixbuf = gdk_pixbuf_scale_simple(pixbuf, preview_width, preview_height, GDK_INTERP_BILINEAR);
   gtk_image_set_from_pixbuf (GTK_IMAGE (preview), previewPixbuf);
  
   gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(chooser), preview);   
   g_object_unref (previewPixbuf);

   gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);
   
   gchar* filename = get_default_name();

   gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);

   if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      /* store the folder location this will be proposed the next time */
      g_free(*workspace_dir);
      *workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser)); 

      gchar* supported_extension = ".pdf";
      g_free(filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      if (g_str_has_suffix(filename, supported_extension))
        {
          pdf_data->filename = g_strdup_printf("%s%s",filename,supported_extension);
        }      
      else
        {
          pdf_data->filename = g_strdup_printf("%s",filename);
        }
      g_free(filename);     

      if (file_exists(pdf_data->filename))
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
		ret = FALSE;
	    } 
	}
    }
  
  gtk_widget_destroy (preview);
  if (chooser != NULL)
    {
      gtk_widget_destroy (chooser);
    }
  return ret;
}


/* Initialize the pdf saver */
gboolean init_pdf_saver(GtkWindow *parent, gchar** workspace_dir, GdkPixbuf *pixbuf)
{  
   pdf_data = (PdfData *) g_malloc(sizeof(PdfData));   
   pdf_data->pixbuflist = NULL;
   pdf_data->tid = -1;
   
   /* start the widget to ask the file name where save the pdf */       
   gboolean ret = start_save_pdf_dialog(parent, workspace_dir, pixbuf);

   if (!ret)
     {
        
        return FALSE;
     }
    
   return TRUE;
}


void *pdf_save(void *arg)
{
   /* begin critical section */
   /* wait if there is a pending thread */
   if (pdf_data->tid!=-1)
     {
       pthread_join(pdf_data->tid, NULL);
     }
   
   gint height = gdk_screen_height ();
   gint width = gdk_screen_width ();
   
   /* create the cairo surface for pdf */
   cairo_surface_t* pdf_surface = cairo_pdf_surface_create(pdf_data->filename, width, height);
   cairo_t* pdf_cr = cairo_create(pdf_surface);

   gint lenght = g_slist_length(pdf_data->pixbuflist);
   
   gint i;
   for (i=lenght-1; i>=0; i--)
     {
        GdkPixbuf* current_pixbuf = (GdkPixbuf*) g_slist_nth_data (pdf_data->pixbuflist, i);
  
        gdk_cairo_set_source_pixbuf(pdf_cr, current_pixbuf, 0, 0);
        cairo_paint(pdf_cr);
        cairo_show_page(pdf_cr);
     }

   cairo_surface_flush(pdf_surface);  
   /* destroy */
   cairo_surface_destroy(pdf_surface);
   cairo_destroy(pdf_cr);
   pdf_data->tid = -1;
   pthread_exit(NULL);
   /* end critical section */
}


/* Add the screenshot to pdf */
void add_pdf_page(GtkWindow *parent, gchar** workspace_dir)
{
   GdkPixbuf* pixbuf = grab_screenshot();
   if (pdf_data == NULL)
     {
        if (!init_pdf_saver(parent, workspace_dir, pixbuf))
          {  
             g_object_unref (pixbuf);
             return;
          }
      }
  
   pdf_data->pixbuflist = g_slist_prepend(pdf_data->pixbuflist, pixbuf);  
  
   // start save thread
   pthread_create(&pdf_data->tid, NULL, pdf_save, (void *) NULL);

}


/* Quit the pdf saver */
void quit_pdf_saver()
{
   if (pdf_data)
     {
        // wait if there is a pending thread
        if (pdf_data->tid!=-1)
          {
             pthread_join(pdf_data->tid, NULL);
          }
        /* free the list and all the pixbuf inside it */
        g_slist_foreach(pdf_data->pixbuflist, (GFunc)g_object_unref, NULL);
        g_slist_free(pdf_data->pixbuflist);
        g_free(pdf_data);
     }
}

