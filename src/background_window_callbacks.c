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


#include <background_window_callbacks.h>
#include <background_window.h>
#include <utils.h> 

/* On configure event. */
G_MODULE_EXPORT gboolean
on_back_configure                 (GtkWidget       *widget,
                                   GdkEventExpose  *event,
                                   gpointer        user_data)
{
  BackgroundData *background_data = (BackgroundData *) user_data;

  GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (widget));
  gint is_fullscreen = state & GDK_WINDOW_STATE_FULLSCREEN;
  if (!is_fullscreen)
    {
      return FALSE;
    }

  if (!background_data->background_cr)
    {
      background_data->background_cr = gdk_cairo_create (gtk_widget_get_window (widget) );
    }

  return TRUE;
}


/* On screen changed. */
G_MODULE_EXPORT void
on_back_screen_changed            (GtkWidget  *widget,
                                   GdkScreen  *previous_screen,
                                   gpointer    user_data)
{
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET (widget));
  GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
  if (visual == NULL)
    {
      visual = gdk_screen_get_system_visual (screen);
    }
    
  gtk_widget_set_visual (widget, visual);
}


/* Expose event in background window occurs. */
G_MODULE_EXPORT gboolean
back_event_expose                 (GtkWidget  *widget,
                                   cairo_t    *cr,
                                   gpointer user_data)
{
  BackgroundData *background_data = (BackgroundData *) user_data;
  
  if ((background_data->background_image) && (get_background_type () == 2))
    {
      update_background_image (background_data->background_image);
    }
  else if ((background_data->background_color) && (get_background_type () == 1))
    {
      update_background_color (background_data->background_color);
    }
  else
    {
      clear_background_window ();
    }
  return TRUE;
}


