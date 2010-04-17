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

#define COLORSIZE 9

/* annotation is visible */
gboolean     visible = TRUE;

/* pencil is selected */
gboolean     pencil = TRUE;

/* grab when leave */
gboolean     grab = FALSE;

/* Is the text inserction enabled */
gboolean text = FALSE;

/* selected color in RGBA format */
gchar*       color = NULL;

/* selected line width */
int          tickness = 15;

/* highlighter flag */
gboolean     highlighter = FALSE;

/* rectifier flag */
gboolean     rectifier = FALSE;

/* rounder flag */
gboolean     rounder = FALSE;

/* arrow=0 mean no arrow, arrow=1 mean normal arrow, arrow=2 mean double arrow */
int        arrow = 0;

/* Default folder where store images and videos */
char*       workspace_dir = NULL;



/* Called when close the program */
gboolean  quit()
{
  gboolean ret = FALSE;
  quit_recorder();
  clear_background(get_annotation_window());
  annotate_quit();
  if (gtkBuilder)
  {
    gtk_widget_destroy(get_bar_window());
    g_object_unref (gtkBuilder); 
  }
  g_free(color);
  g_free(workspace_dir);
  gtk_main_quit();
  exit(ret);
}


/* Add alpha channel to build the RGBA string */
void add_alpha(char *color)
{
  if (highlighter)
    {
      strncpy(&color[6], "88", 2);
    }
  else
    {
      strncpy(&color[6], "FF", 2);
    }
  color[8]=0;
}


/* free color */
void set_color(char* selected_color)
{
  grab = TRUE;
  strcpy(color, selected_color);
  
  add_alpha(color);
  
  annotate_set_color(color);
}


/* Start to annotate calling annotate */
void annotate()
{
  annotate_set_rectifier(rectifier);
  
  annotate_set_rounder(rounder);
  
  annotate_set_width(tickness);

  annotate_set_arrow(arrow);

  annotate_toggle_grab();
}


/* Start to erase calling annotate */
void erase()
{
  annotate_set_width(tickness); 
  annotate_eraser_grab();
}


/* Start to paint with the selected tool */
void start_tool()
{
  if (grab)
    {
      if (text)
	{
          annotate_release_grab();
	  start_text_widget( GTK_WINDOW(get_bar_window()), color, tickness);
	}
      else 
	{
	  if (pencil)
	    { 
	      annotate();
	    }
	  else
	    {
	      erase();
	    }   
	  grab = FALSE;
	}
    }
}


/* Start event handler section */


G_MODULE_EXPORT gboolean
on_window_configure_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if (color == NULL)
    {
      color = malloc(COLORSIZE);
      set_color("FF0000");
      add_alpha(color);
    }
  return TRUE;
}


/* Called when push the quit button */
G_MODULE_EXPORT gboolean
on_quit                          (GtkWidget       *widget,
                                  GdkEvent        *event,
                                  gpointer         user_data)
{
  return quit();
}


/* Called when push the info button */
G_MODULE_EXPORT gboolean
on_info                         (GtkToolButton   *toolbutton,
			         gpointer         user_data)
{
  grab = FALSE;
  start_info_dialog(toolbutton, GTK_WINDOW(get_bar_window()));
  grab = TRUE;
  start_tool();
  return TRUE;
}


/* Called when leave the window */
G_MODULE_EXPORT gboolean
on_winMain_leave_notify_event   (GtkWidget       *widget,
			         GdkEvent        *event,
			         gpointer         user_data)
{
  start_tool();
  return TRUE;
}


/* Called when enter the window */
G_MODULE_EXPORT gboolean
on_winMain_enter_notify_event   (GtkWidget       *widget,
				 GdkEvent        *event,
			         gpointer         user_data)
{
  stop_text_widget();
  return TRUE;
}


G_MODULE_EXPORT gboolean
on_winMain_delete_event          (GtkWidget       *widget,
                                  GdkEvent        *event,
                                  gpointer         user_data)
{
  return quit();
}


G_MODULE_EXPORT void
on_toolsHighlighter_activate     (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      highlighter = TRUE;
    }
  else
    {
      highlighter = FALSE;
    }
  add_alpha(color);
}


G_MODULE_EXPORT void
on_toolsRectifier_activate       (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if rounder is active release it */
      GtkToggleToolButton* rounderToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonRounder"));
      if (gtk_toggle_tool_button_get_active(rounderToolButton))
        {
	  gtk_toggle_tool_button_set_active(rounderToolButton, FALSE); 
          rounder = FALSE;
        }
      rectifier = TRUE;
    }
  else
    {
      rectifier = FALSE;
    }
}


G_MODULE_EXPORT void
on_toolsRounder_activate         (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = TRUE;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if rectifier is active release it */
      GtkToggleToolButton* rectifierToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonRectifier"));
      if (gtk_toggle_tool_button_get_active( rectifierToolButton))
        {
	  gtk_toggle_tool_button_set_active( rectifierToolButton, FALSE); 
          rectifier = FALSE;
        }
      rounder = TRUE;
    }
  else
    {
      rounder = FALSE;
    }
}


G_MODULE_EXPORT void
on_toolsFiller_activate          (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = TRUE;
  annotate_fill();
}


G_MODULE_EXPORT void
on_toolsArrow_activate           (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = TRUE;
  text = FALSE;
  pencil = TRUE;
  arrow = 1;
}


G_MODULE_EXPORT void
on_toolsDoubleArrow_activate     (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  grab = TRUE;
  text = FALSE;
  pencil = TRUE;
  arrow = 2;
}


G_MODULE_EXPORT void
on_toolsText_activate            (GtkToolButton   *toolbutton,
		                  gpointer         user_data)
{
  grab = TRUE;
  text = TRUE;
  arrow = 0;
}


G_MODULE_EXPORT void
on_toolsPencil_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  grab = TRUE;
  text = FALSE;
  pencil = TRUE;
  arrow = 0;
}


G_MODULE_EXPORT void
on_toolsEraser_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  text = FALSE;
  grab = TRUE;
  pencil = FALSE;
  arrow = 0;
}


G_MODULE_EXPORT void
on_toolsVisible_activate         (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  if (visible)
    {
      annotate_hide_annotation();
      grab = FALSE;
      visible=FALSE;
      /* set tooltip to unhide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Unhide");
    }
  else
    {
      annotate_show_annotation();
      visible=TRUE;
      grab = TRUE;
      /* set tooltip to hide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Hide");
    }
}


G_MODULE_EXPORT void
on_toolsScreenShot_activate	 (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  grab = FALSE;
  start_save_image_dialog(toolbutton, GTK_WINDOW(get_bar_window()), &workspace_dir);
  grab = TRUE;
  start_tool();
}


G_MODULE_EXPORT void
on_toolsRecorder_activate        (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{ 
  grab = FALSE;
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
      gboolean status = start_save_video_dialog(toolbutton, GTK_WINDOW(get_bar_window()), &workspace_dir);
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
  grab = TRUE;
  start_tool();
}


G_MODULE_EXPORT void
on_thickScale_value_changed      (GtkHScale   *hScale,
			          gpointer         user_data)
{
  grab = TRUE;
  tickness=gtk_range_get_value(&hScale->scale.range);
}


G_MODULE_EXPORT void
on_toolsPreferences_activate	 (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  grab = FALSE;
  start_preference_dialog(toolbutton, GTK_WINDOW(get_bar_window()));
  grab = TRUE;
  start_tool();
}


G_MODULE_EXPORT void
on_buttonUnlock_activate         (GtkToolButton   *toolbutton,
				  gpointer         user_data)
{
  grab = FALSE;
  annotate_release_grab ();
}


G_MODULE_EXPORT void
on_buttonUndo_activate           (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  grab = TRUE;
  annotate_undo();
}


G_MODULE_EXPORT void
on_buttonRedo_activate           (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  grab = TRUE;
  annotate_redo();
}


G_MODULE_EXPORT void
on_buttonClear_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  grab = TRUE;
  annotate_clear_screen (); 
}


G_MODULE_EXPORT void
on_buttonColor_activate	         (GtkToolButton   *toolbutton,
			          gpointer         user_data)
{
  grab = FALSE;
  pencil = TRUE;
  set_color(start_color_selector_dialog(toolbutton, GTK_WINDOW(get_bar_window()), color));
  grab = TRUE;
  start_tool();
}


/* Start color handlers */

G_MODULE_EXPORT void
on_colorBlack_activate           (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("000000");
}


G_MODULE_EXPORT void
on_colorBlue_activate            (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("3333CC");
}


G_MODULE_EXPORT void
on_colorRed_activate             (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("FF0000");
}


G_MODULE_EXPORT void
on_colorGreen_activate           (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("008000");
}


G_MODULE_EXPORT void
on_colorLightBlue_activate       (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("00C0FF");
}


G_MODULE_EXPORT void
on_colorLightGreen_activate      (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("00FF00");
}


G_MODULE_EXPORT void
on_colorMagenta_activate         (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("FF00FF");
}


G_MODULE_EXPORT void
on_colorOrange_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("FF8000");
}


G_MODULE_EXPORT void
on_colorYellow_activate          (GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
  set_color("FFFF00");
}


G_MODULE_EXPORT void
on_colorWhite_activate           (GtkToolButton   *toolbutton,
	       			  gpointer         user_data)
{
  set_color("FFFFFF");
}


G_MODULE_EXPORT void
destroy (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}


