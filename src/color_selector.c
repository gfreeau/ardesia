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
#include "utils.h"


/* old picked color in RGBA format */
gchar*       picked_color = NULL;

/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 * it return the selected color
 */
gchar* start_color_selector_dialog(GtkToolButton   *toolbutton, GtkWindow *parent, gchar* workspace_dir, gchar* color)
{
  GtkToggleToolButton *button = GTK_TOGGLE_TOOL_BUTTON(toolbutton);
 
  if (gtk_toggle_tool_button_get_active(button))
    {

      /* open color widget */
      GtkWidget* colorDialog = gtk_color_selection_dialog_new ("Changing color");
      
      gtk_window_set_transient_for(GTK_WINDOW(colorDialog), parent);
      gtk_window_stick((GtkWindow*)colorDialog);

      GtkColorSelection *colorsel = GTK_COLOR_SELECTION ((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
    
      /* color initially selected */ 
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor));
      gchar    *ccolor = malloc(strlen(color)+2);
      if (picked_color != NULL)
        {
           strncpy(&ccolor[1],picked_color,strlen(picked_color)+1);
        }
      else
        {
           strncpy(&ccolor[1],color,strlen(color)+1); 
        }
      ccolor[0]='#'; 
      gdk_color_parse (ccolor, gdkcolor);
      g_free(ccolor);

      gtk_color_selection_set_current_color(colorsel, gdkcolor);
      gtk_color_selection_set_previous_color(colorsel, gdkcolor);
      gtk_color_selection_set_has_palette(colorsel, TRUE);

      gint result = gtk_dialog_run((GtkDialog *) colorDialog);

      /* Wait for user to select OK or Cancel */
      switch (result)
	{
	case GTK_RESPONSE_OK:
	  colorsel = GTK_COLOR_SELECTION ((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
          gtk_color_selection_set_has_palette(colorsel, TRUE);
          gtk_color_selection_get_current_color   (colorsel, gdkcolor);
          color = gdkcolor_to_rgb(gdkcolor);
          if (picked_color == NULL)
            {
	       picked_color = malloc(strlen(color));
            }
          strncpy(picked_color, color, strlen(color));
          g_free(gdkcolor);
	  break;
	default:
	  break;
	}
      if (colorDialog != NULL)
      {
        gtk_widget_destroy(colorDialog);
      }
    }
   return color;
}
