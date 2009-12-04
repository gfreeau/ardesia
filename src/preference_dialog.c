/* 
 * Annotate -- a program for painting on the screen 
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
#include "background.h"
#include "interface.h"
#include "utils.h"

/* Preference dialog */
GtkBuilder*  dialogGtkBuilder = NULL;

/* 0 no background, 1 background color, 2 png background, */
int 	     background = 0;

/* preview of background file */
GtkWidget*   preview;

/* Update the preview image */
void on_imageChooserButton_update_preview (GtkFileChooser *file_chooser, gpointer data)
{
  char *filename;
  GdkPixbuf *pixbuf;
  gboolean have_preview;

  filename = gtk_file_chooser_get_preview_filename (file_chooser);
  
  if (filename!=NULL)
    {
      pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
      have_preview = (pixbuf != NULL);
      g_free (filename);

      gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
      if (pixbuf)
        {  
          gdk_pixbuf_unref (pixbuf);
        }

      gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
    }
}

/* Shot when the ok button in preference dialog is pushed */
void on_preferenceOkButton_clicked(GtkButton *buton, gpointer user_date)
{
 GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
  if (gtk_toggle_button_get_active(colorToolButton))
    {
      /* background color */
      GtkColorButton* backgroundColorButton = GTK_COLOR_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"backgroundColorButton"));
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor)); 
      gtk_color_button_get_color(backgroundColorButton,gdkcolor);
      char* rgb = gdkcolor_to_rgb(gdkcolor);
      char* a = malloc(3);
      sprintf(a,"%02x", gtk_color_button_get_alpha (backgroundColorButton)/257);
      change_background_color(rgb,a);
      g_free(gdkcolor);
      background = 1;  
    }
  else 
    {
      GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
      if (gtk_toggle_button_get_active(imageToolButton))
	{
	  /* background png from file */
	  GtkFileChooserButton* imageChooserButton = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"imageChooserButton"));
	  gchar* file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(imageChooserButton)); 
	  if (file != NULL)
            {
              change_background_image(file);
              g_free(file);
              background = 2;  
            }
          else
            {
              /* no background */
	      clear_background();
              background = 0;  
            }
	}
      else
	{
	  /* none */
	  clear_background();  
          background = 0;  
	} 
    }        
  GtkWidget *preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  if (preferenceDialog != NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
}


/* Shot when the selected folder change in the file browser */
void on_imageChooserButton_file_set (GtkButton *buton, gpointer user_date)
{
 GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
 gtk_toggle_button_set_active(imageToolButton,TRUE);
}


/* Shot when is pushed the background color button */
void on_backgroundColorButton_color_set (GtkButton *buton, gpointer user_date)
{
 GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
 gtk_toggle_button_set_active(colorToolButton,TRUE);
}


/* Shot when is pushed the cancel button */
void on_preferenceCancelButton_clicked    (GtkButton *buton,
                                           gpointer user_date)
{
  GtkWidget *preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  if (preferenceDialog != NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
}


/*
 * Start the dialog that ask to the user
 * the background setting
 */
void start_preference_dialog(GtkToolButton   *toolbutton, GtkWindow *parent, char *installation_location)
{
  GtkWidget *preferenceDialog;

  /* Initialize the main window */
  dialogGtkBuilder= gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gchar* name = "preferenceDialog.ui";
  gchar* ui_location =  (gchar *) malloc((strlen(installation_location) + strlen(name) + 1 )* sizeof(gchar));
  sprintf(ui_location, "%s%s", installation_location, name);
  gtk_builder_add_from_file(dialogGtkBuilder, ui_location, NULL);
  free(ui_location);
 
  
  /* Fill the window by the gtk builder xml */
  preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  gtk_window_set_transient_for(GTK_WINDOW(preferenceDialog), parent);
  gtk_window_stick((GtkWindow*)preferenceDialog);
  
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(dialogGtkBuilder,"imageChooserButton"));
  gchar* default_dir_name = "backgrounds";
  char* default_dir = malloc((strlen(installation_location) + strlen(default_dir_name) + 2 )* sizeof(gchar));
  sprintf(default_dir, "%s%s", installation_location, default_dir_name);

  gtk_file_chooser_set_current_folder(chooser, default_dir);
  free(default_dir); 
 
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG and JPEG");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);
 
  preview = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser, preview);
 
  GtkWidget* color_button = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"backgroundColorButton"));
  gtk_color_button_set_use_alpha      (GTK_COLOR_BUTTON(color_button), TRUE);
 
  /* Connect all signals by reflection */
  gtk_builder_connect_signals ( dialogGtkBuilder, NULL );

  GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
  if (background==1)
  {
     gtk_toggle_button_set_active(colorToolButton,TRUE);
  }
  else if (background==2)
  {
     gtk_toggle_button_set_active(imageToolButton,TRUE);
  }

  gtk_dialog_run(GTK_DIALOG(preferenceDialog));
  if (preferenceDialog!=NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
}


