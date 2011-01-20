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

/* Update the preview image */
G_MODULE_EXPORT void
on_imageChooserButton_update_preview (GtkFileChooser *file_chooser, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  gchar *filename;
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


/* Shot when the selected folder change in the file browser */
G_MODULE_EXPORT void
on_imageChooserButton_file_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  GObject* fileObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"file");
  GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(fileObj);
  gtk_toggle_button_set_active(imageToolButton, TRUE);
}


/* Shot when is pushed the background color button */
G_MODULE_EXPORT void
on_backgroundColorButton_color_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  GObject* colorObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"color");
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(colorObj);
  gtk_toggle_button_set_active(colorToolButton, TRUE);
}


/* Shot when the ok button in preference dialog is pushed */
G_MODULE_EXPORT void
on_preferenceOkButton_clicked(GtkButton *buton, gpointer data)
{
  PreferenceData *preferenceData = (PreferenceData*) data;
  GObject* colorToolObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder, "color");
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(colorToolObj);
  if (gtk_toggle_button_get_active(colorToolButton))
    {
      /* background color */
      GObject* backgroundColorObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"backgroundColorButton");

      GtkColorButton* backgroundColorButton = GTK_COLOR_BUTTON(backgroundColorObj);
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor)); 
      gtk_color_button_get_color(backgroundColorButton,gdkcolor);

      gchar* rgb = gdkcolor_to_rgb(gdkcolor);

      gchar* a = g_strdup_printf("%02x", gtk_color_button_get_alpha (backgroundColorButton)/257);
      gchar* rgba = g_strdup_printf("%s%s", rgb, a);
      change_background_color(rgba);
      g_free(a);
      g_free(rgb);
      g_free(rgba);
      g_free(gdkcolor);
      preferenceData->background = 1;  
    }
  else 
    {
      GObject* fileObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"file");
      GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(fileObj);
      if (gtk_toggle_button_get_active(imageToolButton))
	{
	  /* background png from file */
          GObject* imageObj = gtk_builder_get_object(preferenceData->preferenceDialogGtkBuilder,"imageChooserButton");
	  GtkFileChooserButton* imageChooserButton = GTK_FILE_CHOOSER_BUTTON(imageObj);
	  gchar* file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(imageChooserButton)); 
	  if (file != NULL)
            {
              change_background_image(file);
  
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


/* Shot when is pushed the cancel button */
G_MODULE_EXPORT void
on_preferenceCancelButton_clicked    (GtkButton *buton,
				      gpointer data)
{
  /* do nothing */
}

