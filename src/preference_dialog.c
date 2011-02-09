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
#  include <config.h>
#endif

#include <preference_dialog.h>
#include <annotation_window.h>
#include <keyboard.h>


/*
 * Start the dialog that ask to the user
 * the background setting.
 */
void start_preference_dialog(GtkWindow *parent)
{
  PreferenceData *preference_data = (PreferenceData *) g_malloc((gsize) sizeof(PreferenceData));
  GtkWidget *preference_dialog;
  
  start_virtual_keyboard();

  /* 0 no background, 1 background color, 2 png background. */
  preference_data->background = 0;


  /* Initialize the main window. */
  preference_data->preference_dialog_gtk_builder = gtk_builder_new();

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file(preference_data->preference_dialog_gtk_builder, PREFERENCE_UI_FILE, NULL);
 
  /* Fill the window by the gtk builder xml. */
  preference_dialog = GTK_WIDGET(gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "preferences"));
  gtk_window_set_transient_for(GTK_WINDOW(preference_dialog), parent);
  gtk_window_set_modal(GTK_WINDOW(preference_dialog), TRUE);
  
#ifdef _WIN32
  /* 
   * In Windows the parent bar go above the dialog;
   * to avoid this behaviour I put the parent keep above to false.
   */
  gtk_window_set_keep_above(GTK_WINDOW(parent), FALSE);
#endif 
   
  GObject* img_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "imageChooserButton");
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(img_obj);


  gtk_file_chooser_set_current_folder(chooser, BACKGROUNDS_FOLDER);
 
  /* Put the file filter for the supported formats. */
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG and JPEG");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);
 
  preference_data->preview = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser, preference_data->preview);
 
  GtkWidget* color_button = GTK_WIDGET(gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "backgroundColorButton"));
  gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON(color_button), TRUE);
 
  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (preference_data->preference_dialog_gtk_builder, (gpointer) preference_data);
   
  if (preference_data->background == 1)
    {
      GObject * color_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "color");
      GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(color_obj);
      gtk_toggle_button_set_active(colorToolButton, TRUE);
    }
  else if (preference_data->background == 2)
    {
      GObject* file_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "file");
      GtkToggleButton* image_tool_button = GTK_TOGGLE_BUTTON(file_obj);
      gtk_toggle_button_set_active(image_tool_button, TRUE);
    }

  gtk_dialog_run(GTK_DIALOG(preference_dialog));
  
  if (preference_dialog != NULL)
    {
      gtk_widget_destroy(preference_dialog);
    }

  g_object_unref (preference_data->preference_dialog_gtk_builder);
  g_free(preference_data);

#ifdef _WIN32
  /* 
   * Reput the keep above flag at the parent window bar.
   */
  gtk_window_set_keep_above(GTK_WINDOW(parent), TRUE);
#endif
  stop_virtual_keyboard();   
}


