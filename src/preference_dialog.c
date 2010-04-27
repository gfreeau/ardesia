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
#include "background.h"
#include "annotate.h"
#include "utils.h"


#ifdef _WIN32
  #include <gdkwin32.h>
  #define PREFERENCE_UI_FILE "preference_dialog.glade"
  #define BACKGROUNDS_FOLDER "backgrounds"
#else
  #define PREFERENCE_UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/preference_dialog.glade"
  #define BACKGROUNDS_FOLDER PACKAGE_DATA_DIR"/ardesia/ui/backgrounds"
#endif 

typedef struct
{
  /* Preference dialog */
  GtkBuilder*  preferenceDialogGtkBuilder;

  /* 0 no background, 1 background color, 2 png background, */
  int 	     background;

  /* preview of background file */
  GtkWidget*   preview;
}PreferenceData;



/* Update the preview image */
G_MODULE_EXPORT void
on_imageChooserButton_update_preview (GtkFileChooser *file_chooser, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  char *filename;
  GdkPixbuf *pixbuf;
  gboolean have_preview;

  filename = gtk_file_chooser_get_preview_filename (file_chooser);
  
  if (filename != NULL)
    {
      pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
      have_preview = (pixbuf != NULL);
      g_free (filename);

      gtk_image_set_from_pixbuf (GTK_IMAGE (preferenceData->preview), pixbuf);
      if (pixbuf)
        {  
          gdk_pixbuf_unref (pixbuf);
        }

      gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
    }
}


/* Shot when the ok button in preference dialog is pushed */
G_MODULE_EXPORT void
on_preferenceOkButton_clicked(GtkButton *buton, gpointer data)
{
 PreferenceData *preferenceData = (PreferenceData*) data;
 GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder, "color"));
  if (gtk_toggle_button_get_active(colorToolButton))
    {
      /* background color */
      GtkColorButton* backgroundColorButton = GTK_COLOR_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"backgroundColorButton"));
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor)); 
      gtk_color_button_get_color(backgroundColorButton,gdkcolor);

      char* rgb = gdkcolor_to_rgba(gdkcolor);
      char* a = g_malloc(3);
      sprintf(a,"%02x", gtk_color_button_get_alpha (backgroundColorButton)/257);
      strncpy(&rgb[6], a, 2);
      change_background_color(rgb);
      g_free(a);
      g_free(rgb);
      g_free(gdkcolor);
      preferenceData->background = 1;  
    }
  else 
    {
      GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"file"));
      if (gtk_toggle_button_get_active(imageToolButton))
	{
	  /* background png from file */
	  GtkFileChooserButton* imageChooserButton = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"imageChooserButton"));
	  gchar* file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(imageChooserButton)); 
	  if (file != NULL)
            {
              change_background_image(file);
              g_free(file);
              preferenceData->background = 2;  
            }
          else
            {
              /* no background */
	      clear_background_window();
              preferenceData->background = 0;  
            }
	}
      else
	{
	  /* none */
	  clear_background_window();  
          preferenceData->background = 0;  
	} 
    }        
}


/* Shot when the selected folder change in the file browser */
G_MODULE_EXPORT void
on_imageChooserButton_file_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"file"));
  gtk_toggle_button_set_active(imageToolButton,TRUE);
}


/* Shot when is pushed the background color button */
G_MODULE_EXPORT void
on_backgroundColorButton_color_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"color"));
  gtk_toggle_button_set_active(colorToolButton,TRUE);
}


/* Shot when is pushed the cancel button */
G_MODULE_EXPORT void
on_preferenceCancelButton_clicked    (GtkButton *buton,
                                           gpointer data)
{
  /* do nothing */
}


/*
 * Start the dialog that ask to the user
 * the background setting
 */
void start_preference_dialog(GtkToolButton   *toolbutton, GtkWindow *parent)
{
  PreferenceData *preference_data = (PreferenceData *) g_malloc(sizeof(PreferenceData));

  /* 0 no background, 1 background color, 2 png background, */
  preference_data->background = 0;

  GtkWidget *preferenceDialog;

  /* Initialize the main window */
  preference_data->preferenceDialogGtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(preference_data->preferenceDialogGtkBuilder, PREFERENCE_UI_FILE, NULL);
 
  /* Fill the window by the gtk builder xml */
  preferenceDialog = GTK_WIDGET(gtk_builder_get_object(preference_data->preferenceDialogGtkBuilder,"preferences"));
  gtk_window_set_transient_for(GTK_WINDOW(preferenceDialog), parent);
  gtk_window_set_modal(GTK_WINDOW(preferenceDialog), TRUE);

  if (STICK)
    {
      gtk_window_stick((GtkWindow*)preferenceDialog);
    }  

  GtkFileChooser* chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(preference_data->preferenceDialogGtkBuilder, "imageChooserButton"));

  gtk_file_chooser_set_current_folder(chooser, BACKGROUNDS_FOLDER);
 
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG and JPEG");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);
 
  preference_data->preview = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser, preference_data->preview);
 
  GtkWidget* color_button = GTK_WIDGET(gtk_builder_get_object(preference_data->preferenceDialogGtkBuilder,"backgroundColorButton"));
  gtk_color_button_set_use_alpha      (GTK_COLOR_BUTTON(color_button), TRUE);
 
  /* Connect all signals by reflection */
  gtk_builder_connect_signals (preference_data->preferenceDialogGtkBuilder, (gpointer) preference_data);
  GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preference_data->preferenceDialogGtkBuilder,"file"));
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(preference_data->preferenceDialogGtkBuilder,"color"));
  if (preference_data->background == 1)
  {
     gtk_toggle_button_set_active(colorToolButton,TRUE);
  }
  else if (preference_data->background == 2)
  {
     gtk_toggle_button_set_active(imageToolButton,TRUE);
  }

  gtk_dialog_run(GTK_DIALOG(preferenceDialog));
  if (preferenceDialog != NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
  g_object_unref (preference_data->preferenceDialogGtkBuilder);
  g_free(preference_data);
}


