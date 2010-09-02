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


gboolean start_save_pdf_dialog(GtkWindow *parent, gchar** workspace_dir, gchar** filename, GdkPixbuf *pixbuf)
{
   gboolean ret = TRUE;
   if (!(*workspace_dir))
    {
      /* Initialize it to the desktop folder */
      gchar* desktop_dir = (gchar *) get_desktop_dir();
      gint lenght = strlen(desktop_dir);
      *workspace_dir = (gchar*) g_malloc( ( lenght + 1) * sizeof(gchar));
      strcpy(*workspace_dir, desktop_dir);
      g_free(desktop_dir);
    }	
   

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
   
   gchar* start_string = "ardesia_";
   gchar* date = get_date(); 
   
   *filename = (gchar*) g_malloc((strlen(start_string) + strlen(date) + 1) * sizeof(gchar));
   sprintf(*filename,"%s%s", start_string, date);
   g_free(date);

   gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), *filename);

   if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      gchar* supported_extension = ".pdf";
      gchar* extension = strrchr(*filename, '.');
      if ((extension==0) || (strcmp(extension, supported_extension) != 0))
        {
          *filename = (gchar *) realloc(*filename,  (strlen(*filename) + strlen(supported_extension) + 1) * sizeof(gchar)); 
          (void) strcat((gchar *)*filename, supported_extension);
        }           
      if (file_exists(*filename,(gchar *) workspace_dir))
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
   gchar* filename;
   
   /* start the widget to ask the file name where save the pdf */       
   gboolean ret = start_save_pdf_dialog(parent, workspace_dir, &filename, pixbuf);

   if (!ret)
     {
        
        return FALSE;
     }

   pdf_data = (PdfData *) g_malloc(sizeof(PdfData));   
   
   /* create the cairo surface for pdf */
   gint height = gdk_screen_height ();
   gint width = gdk_screen_width ();
   cairo_surface_t* pdf_surface = cairo_pdf_surface_create(filename, width, height);
   g_free(filename);
   pdf_data->cr = cairo_create(pdf_surface);
    
   return TRUE;
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

  /* The surface must load the pixbuf contents */
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *back_surface;
  back_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *back_cr = cairo_create(back_surface);
  gdk_cairo_set_source_pixbuf(pdf_data->cr, pixbuf, 0, 0);
  cairo_paint(pdf_data->cr);

  /* add the back_surface */
  cairo_set_source_surface(pdf_data->cr, back_surface, 0, 0);
  cairo_paint(pdf_data->cr);
  cairo_copy_page(pdf_data->cr);
  cairo_show_page(pdf_data->cr);
  cairo_surface_flush(cairo_get_target(pdf_data->cr));

  cairo_surface_destroy(back_surface);
  cairo_destroy(back_cr);
  g_object_unref (pixbuf);
}


/* Quit the pdf saver */
void quit_pdf_saver()
{
  if (pdf_data->cr)
    {
       cairo_surface_destroy(cairo_get_target(pdf_data->cr));
       cairo_destroy(pdf_data->cr);
    }
}

