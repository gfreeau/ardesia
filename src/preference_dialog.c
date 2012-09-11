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

#include <utils.h>
#include <preference_dialog.h>
#include <background_window.h>
#include <annotation_window.h>
#include <keyboard.h>


/* Show the permission denied to access to file dialog. */
void
show_permission_denied_dialog     (GtkWindow *parent)
{
  GtkWidget *permission_denied_dialog = (GtkWidget *) NULL;

  permission_denied_dialog = gtk_message_dialog_new (parent,
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     gettext ("Fail to open the file: Permission denied"));

  gtk_window_set_keep_above (GTK_WINDOW (permission_denied_dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (permission_denied_dialog));

  if (permission_denied_dialog != NULL)
    {
      gtk_widget_destroy (permission_denied_dialog);
      permission_denied_dialog = NULL;
    }
}


/*
 * Start the dialog that ask to the user
 * the background setting.
 */
void
start_preference_dialog      (GtkWindow *parent)
{
  GObject *preference_obj = (GObject *) NULL;
  GtkWidget *preference_dialog = (GtkWidget *) NULL;
  GObject *img_obj = (GObject *) NULL;
  GtkFileChooser *chooser = NULL;
  GtkFileFilter *filter = (GtkFileFilter *) NULL;
  GObject *bg_color_obj = (GObject *) NULL;
  GtkWidget *color_button = (GtkWidget *) NULL;

  PreferenceData *preference_data = (PreferenceData *) NULL;

  start_virtual_keyboard ();

  preference_data = g_malloc ((gsize) sizeof (PreferenceData));

  /* Initialize the main window. */
  preference_data->preference_dialog_gtk_builder = gtk_builder_new ();

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (preference_data->preference_dialog_gtk_builder,
                             PREFERENCE_UI_FILE,
                             NULL);

  /* Take the preference object. */
  preference_obj = gtk_builder_get_object (preference_data->preference_dialog_gtk_builder,
                                           "preferences");

  preference_dialog = GTK_WIDGET (preference_obj);

  gtk_window_set_transient_for (GTK_WINDOW (preference_dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (preference_dialog), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (preference_dialog), TRUE);
  
  img_obj = gtk_builder_get_object (preference_data->preference_dialog_gtk_builder,
                                    "imageChooserButton");

  chooser = GTK_FILE_CHOOSER (img_obj);

  gtk_file_chooser_set_current_folder (chooser, BACKGROUNDS_FOLDER);

  /* Put the file filter for the supported formats. */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG and JPEG");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);

  preference_data->preview = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser, preference_data->preview);

  bg_color_obj = gtk_builder_get_object (preference_data->preference_dialog_gtk_builder,
                                         "backgroundColorButton");

  color_button = GTK_WIDGET (bg_color_obj);
  gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (color_button), TRUE);

  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (preference_data->preference_dialog_gtk_builder,
                               (gpointer) preference_data);

  gint background_type = get_background_type();

  if (background_type == 1)
    {
      GObject *color_obj = gtk_builder_get_object (preference_data->preference_dialog_gtk_builder,
                                                   "color");

      GtkToggleButton *color_tool_button = GTK_TOGGLE_BUTTON (color_obj);
      gtk_toggle_button_set_active (color_tool_button, TRUE);
    }
  else if (background_type == 2)
    {
      GObject *file_obj = gtk_builder_get_object (preference_data->preference_dialog_gtk_builder,
                                                  "file");

      GtkToggleButton *image_tool_button = GTK_TOGGLE_BUTTON (file_obj);
      gtk_toggle_button_set_active (image_tool_button, TRUE);
    }

  gchar *rgba = get_background_color ();
  if (rgba)
    {
      GdkColor *gdkcolor = rgba_to_gdkcolor (rgba);
      guint16 alpha = strtol(&rgba[6], NULL, 16) * 257;
      gtk_color_button_set_alpha (GTK_COLOR_BUTTON (color_button), alpha);
      gtk_color_button_set_color (GTK_COLOR_BUTTON (color_button), gdkcolor);
    }

  gchar *filename = get_background_image ();
  if (filename)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);
    }

  gtk_dialog_run (GTK_DIALOG (preference_dialog));

  if (preference_dialog)
    {
      gtk_widget_destroy (preference_dialog);
      preference_dialog = NULL;
    }

  g_object_unref (preference_data->preference_dialog_gtk_builder);
  preference_data->preference_dialog_gtk_builder = NULL;
  g_free (preference_data);
  preference_data = NULL;

  stop_virtual_keyboard ();
  
  #ifdef _WIN32
  /*
   * In Windows the parent bar go above the dialog;
   * to avoid this behaviour I have put the parent keep above to false,
   * and now I restore it.
   */
  gtk_window_set_keep_above (GTK_WINDOW (parent), TRUE);
#endif
}


