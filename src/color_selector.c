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

#include <color_selector.h>
#include <utils.h>
#include <keyboard.h>


/* old picked color in RGBA format */
static gchar*       picked_color = NULL;


/*
 * Start the color selector dialog 
 * it return the selected color
 */
gchar* start_color_selector_dialog(GtkToolButton *toolbutton, GtkWindow *parent, gchar* color)
{
  start_virtual_keyboard();
  GtkToggleToolButton *button = GTK_TOGGLE_TOOL_BUTTON(toolbutton);
  gchar* ret_color = NULL;
  if (gtk_toggle_tool_button_get_active(button))
    {
      /* open color widget */
      GtkWidget* colorDialog = gtk_color_selection_dialog_new (gettext("Changing color"));
      gtk_window_set_transient_for(GTK_WINDOW(colorDialog), parent);
      gtk_window_set_modal(GTK_WINDOW(colorDialog), TRUE);
      gtk_window_set_keep_above(GTK_WINDOW(colorDialog), TRUE);
	  
      GtkColorSelection *colorsel = GTK_COLOR_SELECTION ((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
    
      /* color initially selected */ 
      GdkColor* gdkcolor;
      if (picked_color != NULL)
        {
	  gdkcolor = rgba_to_gdkcolor(picked_color);
        }
      else
        {
	  gdkcolor = rgba_to_gdkcolor(color);
        }

      gtk_color_selection_set_current_color(colorsel, gdkcolor);
      gtk_color_selection_set_previous_color(colorsel, gdkcolor);
      gtk_color_selection_set_has_palette(colorsel, TRUE);

      gint result = gtk_dialog_run(GTK_DIALOG(colorDialog));

      /* Wait for user to select OK or Cancel */
      switch (result)
	{
	case GTK_RESPONSE_OK:
	  colorsel = GTK_COLOR_SELECTION((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
	  gtk_color_selection_set_has_palette(colorsel, TRUE);
	  gtk_color_selection_get_current_color(colorsel, gdkcolor);
	  ret_color = gdkcolor_to_rgb(gdkcolor);
	  g_free(picked_color);
	  picked_color = g_strdup_printf("%s%s", ret_color, "FF");
	  break;

	default:
	  break;
	}

      if (colorDialog != NULL)
	{
	  gtk_widget_destroy(colorDialog);
	}
     
      g_free(gdkcolor);
    }
  stop_virtual_keyboard();
  return ret_color;
}


