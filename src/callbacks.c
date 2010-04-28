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

#include "ardesia.h"
#include "callbacks.h"


#include <gtk/gtk.h>

#include "recorder.h"
#include "saver.h"
#include "color_selector.h"
#include "preference_dialog.h"
#include "info_dialog.h"
#include "text_widget.h"

#include "annotate.h"

#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 

#include <math.h>
#include <utils.h>
#include <background.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(_WIN32)
  #include <gdkwin32.h>
#endif


/* Called when close the program */
gboolean  quit(BarData *bar_data)
{
  /* Disallocate all the BarData */
  
  /*
  if (bar_data)
    {
      g_free(bar_data->color);
      if (bar_data->workspace_dir)
      {
        g_free(bar_data->workspace_dir);
      }
      g_free(bar_data);
    }
  */
  quit_recorder();
  destroy_background_window();
  annotate_quit();
  if (gtkBuilder)
    {
      g_object_unref (gtkBuilder); 
    }
  gtk_main_quit();
  exit(0);
}


/* Add alpha channel to build the RGBA string */
void add_alpha(BarData *bar_data)
{
  if (bar_data->highlighter)
    {
      strncpy(&bar_data->color[6], "88", 2);
    }
  else
    {
      strncpy(&bar_data->color[6], "FF", 2);
    }
  bar_data->color[8]=0;
}


/* free color */
void set_color(BarData *bar_data, char* selected_color)
{
  bar_data->grab = TRUE;
  strcpy(bar_data->color, selected_color);
  
  add_alpha(bar_data);
  
  annotate_set_color(bar_data->color);
}


/* Start to annotate calling annotate */
void annotate(BarData *bar_data)
{
  annotate_set_rectifier(bar_data->rectifier);
  
  annotate_set_rounder(bar_data->rounder);
  
  annotate_set_width(bar_data->thickness);

  annotate_set_arrow(bar_data->arrow);

  annotate_toggle_grab();
}


/* Start to erase calling annotate */
void erase(BarData *bar_data)
{
  annotate_set_width(bar_data->thickness); 
  annotate_eraser_grab();
}


/* Start to paint with the selected tool */
void start_tool(BarData *bar_data)
{
  if (bar_data->grab)
    {
      if (bar_data->text)
	  {
            annotate_release_grab();
	    start_text_widget( GTK_WINDOW(get_annotation_window()), bar_data->color, bar_data->thickness);
	  }
      else 
	  {
	    if (bar_data->pencil)
	      { 
	        annotate(bar_data);
	      }
	    else
	      {
	        erase(bar_data);
	      }   
	    bar_data->grab = FALSE;
	  }
    }
}


/* Called when push the quit button */
G_MODULE_EXPORT gboolean
on_quit                          (GtkWidget       *widget,
                                  GdkEvent        *event,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  return quit(bar_data);
}


/* Called when push the info button */
G_MODULE_EXPORT gboolean
on_info                         (GtkToolButton   *toolbutton,
			         gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  start_info_dialog(toolbutton, GTK_WINDOW(get_annotation_window()));
  bar_data->grab = TRUE;
  start_tool(bar_data);
  return TRUE;
}


/* Called when leave the window */
G_MODULE_EXPORT gboolean
on_winMain_leave_notify_event   (GtkWidget       *widget,
			         GdkEvent        *event,
			         gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  start_tool(bar_data);
  return TRUE;
}


/* Called when enter the window */
G_MODULE_EXPORT gboolean
on_winMain_enter_notify_event   (GtkWidget       *widget,
				 GdkEvent        *event,
			         gpointer         user_data)
{
  //stop_text_widget();
  return TRUE;
}


G_MODULE_EXPORT gboolean
on_winMain_delete_event          (GtkWidget       *widget,
                                  GdkEvent        *event,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  return quit(bar_data);
}


G_MODULE_EXPORT void
on_toolsHighlighter_activate     (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      bar_data->highlighter = TRUE;
    }
  else
    {
      bar_data->highlighter = FALSE;
    }
  add_alpha(bar_data);
}


G_MODULE_EXPORT void
on_toolsRectifier_activate       (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if rounder is active release it */
      GtkToggleToolButton* rounderToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonRounder"));
      if (gtk_toggle_tool_button_get_active(rounderToolButton))
        {
	  gtk_toggle_tool_button_set_active(rounderToolButton, FALSE); 
          bar_data->rounder = FALSE;
        }
      bar_data->rectifier = TRUE;
    }
  else
    {
      bar_data->rectifier = FALSE;
    }
}


G_MODULE_EXPORT void
on_toolsRounder_activate         (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if rectifier is active release it */
      GtkToggleToolButton* rectifierToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonRectifier"));
      if (gtk_toggle_tool_button_get_active( rectifierToolButton))
        {
	  gtk_toggle_tool_button_set_active( rectifierToolButton, FALSE); 
          bar_data->rectifier = FALSE;
        }
      bar_data->rounder = TRUE;
    }
  else
    {
      bar_data->rounder = FALSE;
    }
}


G_MODULE_EXPORT void
on_toolsFiller_activate          (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  annotate_fill();
}


G_MODULE_EXPORT void
on_toolsArrow_activate           (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->text = FALSE;
  bar_data->pencil = TRUE;
  bar_data->arrow = TRUE;
}


G_MODULE_EXPORT void
on_toolsText_activate            (GtkToolButton   *toolbutton,
		                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->text = TRUE;
  bar_data->arrow = FALSE;
}


G_MODULE_EXPORT void
on_toolsThin_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->thickness = 7;
}


G_MODULE_EXPORT void
on_toolsMedium_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->thickness = 14;
}


G_MODULE_EXPORT void
on_toolsThick_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->thickness = 21;
}


G_MODULE_EXPORT void
on_toolsPencil_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  bar_data->text = FALSE;
  bar_data->pencil = TRUE;
  bar_data->arrow = FALSE;
}


G_MODULE_EXPORT void
on_toolsEraser_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->text = FALSE;
  bar_data->grab = TRUE;
  bar_data->pencil = FALSE;
  bar_data->arrow = FALSE;
}


G_MODULE_EXPORT void
on_toolsVisible_activate         (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  if (bar_data->annotation_is_visible)
    {
      annotate_hide_annotation();
      bar_data->grab = FALSE;
      bar_data->annotation_is_visible=FALSE;
      /* set tooltip to unhide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Unhide");
    }
  else
    {
      annotate_show_annotation();
      bar_data->annotation_is_visible=TRUE;
      bar_data->grab = TRUE;
      /* set tooltip to hide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Hide");
    }
}


G_MODULE_EXPORT void
on_toolsScreenShot_activate	 (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  start_save_image_dialog(toolbutton, GTK_WINDOW(get_annotation_window()), &bar_data->workspace_dir);
  bar_data->grab = TRUE;
  start_tool(bar_data);
}


G_MODULE_EXPORT void
on_toolsRecorder_activate        (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{ 
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  if(is_recording())
    {
      quit_recorder();
      /* set stop tooltip */ 
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, "Record");
      /* put icon to record */
      gtk_tool_button_set_stock_id (toolbutton, "gtk-media-record");
    }
  else
    {      
      /* Release grab */
      annotate_release_grab ();
      /* the recording is not active */ 
      gboolean status = start_save_video_dialog(toolbutton, GTK_WINDOW(get_annotation_window()), &bar_data->workspace_dir);
      if (status)
        {
          /* set stop tooltip */ 
          gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, "Stop");
          /* put icon to stop */
          gtk_tool_button_set_stock_id (toolbutton, "gtk-media-stop");
        }
      else
	{
	  gtk_widget_hide(GTK_WIDGET(toolbutton));
           
	}
    }
  bar_data->grab = TRUE;
  start_tool(bar_data);
}


G_MODULE_EXPORT void
on_toolsPreferences_activate	 (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  start_preference_dialog(toolbutton, GTK_WINDOW(get_annotation_window()));
  bar_data->grab = TRUE;
  start_tool(bar_data);
}


G_MODULE_EXPORT void
on_buttonUnlock_activate         (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  annotate_release_grab ();
}


G_MODULE_EXPORT void
on_buttonUndo_activate           (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  annotate_undo();
}


G_MODULE_EXPORT void
on_buttonRedo_activate           (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  annotate_redo();
}


G_MODULE_EXPORT void
on_buttonClear_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = TRUE;
  annotate_clear_screen (); 
}


G_MODULE_EXPORT void
on_buttonColor_activate	         (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  bar_data->grab = FALSE;
  bar_data->pencil = TRUE;
  set_color(bar_data, start_color_selector_dialog(toolbutton, GTK_WINDOW(get_annotation_window()), bar_data->color));
  bar_data->grab = TRUE;
  start_tool(bar_data);
}


G_MODULE_EXPORT void
on_colorBlue_activate            (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "3333CC");
}


G_MODULE_EXPORT void
on_colorRed_activate             (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "FF0000");
}


G_MODULE_EXPORT void
on_colorGreen_activate           (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "008000");
}


G_MODULE_EXPORT void
on_colorLightGreen_activate      (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "00FF00");
}


G_MODULE_EXPORT void
on_colorYellow_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "FFFF00");
}


G_MODULE_EXPORT void
on_colorWhite_activate           (GtkToolButton   *toolbutton,
	       			  gpointer         user_data)
{
  BarData *bar_data = (BarData*) user_data;
  set_color(bar_data, "FFFFFF");
}


G_MODULE_EXPORT void
destroy (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}


