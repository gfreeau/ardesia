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


#include <utils.h>
#include <preference_dialog.h>
#include <background_window.h>


/* Update the preview image. */
G_MODULE_EXPORT void
on_imageChooserButton_update_preview (GtkFileChooser *file_chooser, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData*) data;
  gchar *filename;
  GdkPixbuf *pixbuf;
  gboolean have_preview;

  filename = gtk_file_chooser_get_preview_filename (file_chooser);
  
  if (filename != NULL)
    {
      pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
      have_preview = (pixbuf != NULL);
      g_free (filename);

      gtk_image_set_from_pixbuf (GTK_IMAGE (preference_data->preview), pixbuf);
      if (pixbuf)
        {  
          gdk_pixbuf_unref (pixbuf);
        }

      gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
    }
}


/* Shot when the selected folder change in the file browser. */
G_MODULE_EXPORT void
on_imageChooserButton_file_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData*) data;
  GObject* file_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "file");
  GtkToggleButton* image_tool_button = GTK_TOGGLE_BUTTON(file_obj);
  gtk_toggle_button_set_active(image_tool_button, TRUE);
}


/* Shot when is pushed the background color button. */
G_MODULE_EXPORT void
on_backgroundColorButton_color_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData*) data;
  GObject* color_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "color");
  GtkToggleButton* color_tool_button = GTK_TOGGLE_BUTTON(color_obj);
  gtk_toggle_button_set_active(color_tool_button, TRUE);
}


/* Shot when the ok button in preference dialog is pushed. */
G_MODULE_EXPORT void
on_preferenceOkButton_clicked(GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData*) data;
  gchar* rgb = NULL;
  gchar* a = NULL;
  gchar* rgba = NULL;
  GObject* color_tool_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "color");
  GtkToggleButton* color_tool_button = GTK_TOGGLE_BUTTON(color_tool_obj);
  
  if (gtk_toggle_button_get_active(color_tool_button))
    {
      /* background color */
      GObject* background_color_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "backgroundColorButton");

      GtkColorButton* background_color_button = GTK_COLOR_BUTTON(background_color_obj);
      GdkColor* gdkcolor = g_malloc ((gsize) sizeof (GdkColor)); 
      gtk_color_button_get_color(background_color_button, gdkcolor);

      rgb = gdkcolor_to_rgb(gdkcolor);
      a = g_strdup_printf("%02x", gtk_color_button_get_alpha (background_color_button)/257);
      rgba = g_strdup_printf("%s%s", rgb, a);
      
      change_background_color(rgba);
      g_free(a);
      g_free(rgb);
      g_free(rgba);
      g_free(gdkcolor);
      preference_data->background = 1;  
    }
  else 
    {
      GObject* file_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "file");
      GtkToggleButton* image_tool_button = GTK_TOGGLE_BUTTON(file_obj);
      if (gtk_toggle_button_get_active(image_tool_button))
	{
	  /* background png from file */
          GObject* image_obj = gtk_builder_get_object(preference_data->preference_dialog_gtk_builder, "imageChooserButton");
	  GtkFileChooserButton* image_chooser_button = GTK_FILE_CHOOSER_BUTTON(image_obj);
	  gchar* file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(image_chooser_button)); 
	  if (file != NULL)
            {
              change_background_image(file);
  
              preference_data->background = 2;  
            }
          else
            {
              /* no background */
	      clear_background_window();
              preference_data->background = 0;
            }
	}
      else
	{
	  /* none */
	  clear_background_window();  
          preference_data->background = 0;  
	} 
    }        
}


/* Shot when is pushed the cancel button. */
G_MODULE_EXPORT void
on_preferenceCancelButton_clicked    (GtkButton *buton,
				      gpointer data)
{
  /* do nothing */
}


