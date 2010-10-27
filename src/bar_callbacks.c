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


#include <bar_callbacks.h>
#include <utils.h>
#include <ardesia.h>
#include <annotate.h>
#include <background.h>
#include <color_selector.h>
#include <preference_dialog.h>
#include <info_dialog.h>
#include <text_widget.h>
#include <recorder.h>
#include <saver.h>
#include <pdf_saver.h>



/* Called when close the program */
gboolean  quit(BarData *bar_data)
{
  quit_pdf_saver();
  annotate_quit();
  quit_recorder();

  /* Disallocate all the BarData */
  if (bar_data)
    {
      g_free(bar_data->color);
      if (bar_data->workspace_dir)
      {
        g_free(bar_data->workspace_dir);
      }
      g_free(bar_data);
    }

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
      strncpy(&bar_data->color[6], SEMI_OPAQUE_ALPHA, 2);
    }
  else
    {
      strncpy(&bar_data->color[6], OPAQUE_ALPHA, 2);
    }
}


/* free color */
void set_color(BarData *bar_data, gchar* selected_color)
{
  strncpy(bar_data->color, selected_color, 6);
  annotate_set_color(bar_data->color);
}


/* Pass the options to annotate */
void set_options(BarData* bar_data)
{
  annotate_set_rectifier(bar_data->rectifier);
  
  annotate_set_rounder(bar_data->rounder);
  
  annotate_set_color(bar_data->color);  

  annotate_set_thickness(bar_data->thickness);
  
  annotate_set_arrow(bar_data->arrow);
 
  if ((bar_data->pencil)||(bar_data->arrow))
    {  
       annotate_select_pen();
    }
  else
    {
       annotate_select_eraser();
    }
}


/* Start to paint with the selected tool */
void start_tool(BarData *bar_data)
{
  if (bar_data->grab)
    {
      if (bar_data->text)
        {
           // text button then start the text widget
           annotate_release_grab();
	   start_text_widget( GTK_WINDOW(get_annotation_window()), bar_data->color, bar_data->thickness);
	}
      else 
	{
          set_options(bar_data);
          annotate_acquire_grab();
	}
    }
}


/* Configure events occurs */
G_MODULE_EXPORT gboolean 
on_window_configure_event (GtkWidget *widget,
			GdkEvent *event,
		 	gpointer func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_options(bar_data);
  return TRUE;
}


/* Called when the main window is destroyed */
G_MODULE_EXPORT void
on_winMain_destroy_event (GtkWidget *widget, gpointer func_data)
{
  BarData *bar_data = (BarData*) func_data;
  quit(bar_data);
}


/* Called when push the quit button */
G_MODULE_EXPORT gboolean
on_quit                          (GtkToolButton   *toolbutton,
			         gpointer         func_data)
{
  /* destroy the backgound window this will call the destroy of all windows */
  destroy_background_window();
  return FALSE;
}


/* Called when push the info button */
G_MODULE_EXPORT gboolean
on_info                         (GtkToolButton   *toolbutton,
			         gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  // start the info dialog
  start_info_dialog(toolbutton, GTK_WINDOW(get_bar_window()));
  bar_data->grab = grab_value;
  start_tool(bar_data);
  return TRUE;
}


/* Called when leave the window */
G_MODULE_EXPORT gboolean
on_winMain_leave_notify_event   (GtkWidget       *widget,
			         GdkEvent        *event,
			         gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  start_tool(bar_data);
  return TRUE;
}


/* Called when enter the window */
G_MODULE_EXPORT gboolean
on_winMain_enter_notify_event   (GtkWidget       *widget,
				 GdkEvent        *event,
			         gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  if (bar_data->text)
    {
      stop_text_widget();
    }
  return TRUE;
}


/* Delete event occurs and then I quit the program */
G_MODULE_EXPORT gboolean
on_winMain_delete_event          (GtkWidget       *widget,
                                  GdkEvent        *event,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  quit(bar_data);
  return FALSE; 
}


/* Push highlighter button */
G_MODULE_EXPORT void
on_toolsHighlighter_activate     (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
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


/* Push rectifier button */
G_MODULE_EXPORT void
on_toolsRectifier_activate       (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
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


/* Push rounder button */
G_MODULE_EXPORT void
on_toolsRounder_activate         (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if rectifier is active release it */
      GObject* rectifier_obj= gtk_builder_get_object(gtkBuilder,"buttonRectifier");
      GtkToggleToolButton* rectifierToolButton = GTK_TOGGLE_TOOL_BUTTON(rectifier_obj);
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


/* Push filler button */
G_MODULE_EXPORT void
on_toolsFiller_activate          (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  annotate_fill();
}


/* Push arrow button */
G_MODULE_EXPORT void
on_toolsArrow_activate           (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  bar_data->text = FALSE;
  bar_data->pencil = TRUE;
  bar_data->arrow = TRUE;
  set_color(bar_data, bar_data->color);
}


/* Push text button */
G_MODULE_EXPORT void
on_toolsText_activate            (GtkToolButton   *toolbutton,
		                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  bar_data->text = TRUE;
  bar_data->arrow = FALSE;
}


/* Push thickness button */
G_MODULE_EXPORT void
on_toolsThick_activate          (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  if (bar_data->thickness== THICK_STEP*2)
    {
       gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"thick")));
       // set thick icon
       bar_data->thickness = THICK_STEP*3;
    }
  else if (bar_data->thickness==THICK_STEP*3)
    {
       // set thin icon
       gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"thin")));
       bar_data->thickness = THICK_STEP;
    }
  else if (bar_data->thickness==THICK_STEP)
    {
       // set medium icon
       gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"medium")));
       bar_data->thickness = THICK_STEP*2;
    }
}


/* Push pencil button */
G_MODULE_EXPORT void
on_toolsPencil_activate          (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  bar_data->text = FALSE;
  bar_data->pencil = TRUE;
  bar_data->arrow = FALSE;
  set_color(bar_data, bar_data->color);
}


/* Push eraser button */
G_MODULE_EXPORT void
on_toolsEraser_activate          (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  bar_data->text = FALSE;
  bar_data->pencil = FALSE;
  bar_data->arrow = FALSE;
  
}


/* Push hide/unhide button */
G_MODULE_EXPORT void
on_toolsVisible_activate         (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  if (bar_data->annotation_is_visible)
    {
      annotate_hide_annotation();
      bar_data->annotation_is_visible = FALSE;
      /* put icon to unhide */
      gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"unhide")));
      /* set tooltip to unhide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Unhide"));
    }
  else
    {
      annotate_show_annotation();
      bar_data->annotation_is_visible = TRUE;
      /* put icon to hide */
      gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"hide")));
      /* set tooltip to hide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Hide"));
    }
}


/* Push save (screenshoot) button */
G_MODULE_EXPORT void
on_toolsScreenShot_activate	 (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  start_save_image_dialog(toolbutton, GTK_WINDOW(get_bar_window()), &bar_data->workspace_dir);
  bar_data->grab = grab_value;
  start_tool(bar_data);
}


/* Add page to pdf */
G_MODULE_EXPORT void
on_toolsAddPdf_activate	 (GtkToolButton   *toolbutton,
                          gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  add_pdf_page(GTK_WINDOW(get_bar_window()), &bar_data->workspace_dir);
  bar_data->grab = grab_value;
  start_tool(bar_data);
}


/* Push recorder button */
G_MODULE_EXPORT void
on_toolsRecorder_activate        (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{ 
  /* Release grab */
  annotate_release_grab ();
  
  BarData *bar_data = (BarData*) func_data;	
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  
  if (!is_recorder_available())	
	{
	  visualize_missing_recorder_program_dialog(GTK_WINDOW(get_bar_window()));
          /* put an icon that remeber that the tool is not available */          
	  gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"media-recorder-unavailable")));
          bar_data->grab = grab_value;
          start_tool(bar_data);		
	  return;
	}
	
  if(is_recording())
    {
      quit_recorder();
      /* set stop tooltip */ 
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Record"));
      /* put icon to record */
      gtk_tool_button_set_stock_id (toolbutton, "gtk-media-record");
    }
  else
    { 
      /* the recording is not active */ 
      gboolean status = start_save_video_dialog(toolbutton, GTK_WINDOW(get_bar_window()), &bar_data->workspace_dir);
      if (status)
        {
          /* set stop tooltip */ 
          gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Stop"));
          /* put icon to stop */
          gtk_tool_button_set_stock_id (toolbutton, "gtk-media-stop");
        }
    }
  bar_data->grab = grab_value;
  start_tool(bar_data);
}


/* Push preference button */
G_MODULE_EXPORT void
on_toolsPreferences_activate	 (GtkToolButton   *toolbutton,
			          gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  start_preference_dialog(toolbutton, GTK_WINDOW(get_bar_window()));
  bar_data->grab = grab_value;
  start_tool(bar_data);
}


/* Push lock/unlock button */
G_MODULE_EXPORT void
on_buttonUnlock_activate         (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  if (bar_data->grab == FALSE)
    {
      bar_data->grab = TRUE;
      #ifdef _WIN32
        /* 
         * @HACK Deny the mouse input to go below the window putting the opacity greater than 0
	 * @TODO remove the opacity hack when will be solved the next todo 
         */
        if (gtk_window_get_opacity(GTK_WINDOW(get_background_window()))==0)
          {
            gtk_window_set_opacity(GTK_WINDOW(get_background_window()), BACKGROUND_OPACITY);
          }
      #endif	
      gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"lock")));
      /* set tooltip to unhide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Unlock"));
    }
  else
    {
      /* grab enabled */
      bar_data->grab = FALSE;
      annotate_release_grab ();
      #ifdef _WIN32
        if (gtk_window_get_opacity(GTK_WINDOW(get_background_window()))!=0)
          {
            /* 
	     * @HACK This allow the mouse input go below the window putting the opacity to 0 
	     * if will be found a better way to see to the window to be transparent
	     * the the pointer input can be removed the previous hack 
	     *
	     * @TODO transparent window to the pointer input in a better way 
	     */
             gtk_window_set_opacity(GTK_WINDOW(get_background_window()), 0);
          }
      #endif	 
      gtk_tool_button_set_label_widget(toolbutton, GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"unlock")));
      /* set tooltip to hide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton, gettext("Lock"));
    }  
}


/* Push undo button */
G_MODULE_EXPORT void
on_buttonUndo_activate           (GtkToolButton   *toolbutton,
			          gpointer         func_data)
{
  annotate_undo();
}


/* Push redo button */
G_MODULE_EXPORT void
on_buttonRedo_activate           (GtkToolButton   *toolbutton,
			          gpointer         func_data)
{
  annotate_redo();
}


/* Push clear button */
G_MODULE_EXPORT void
on_buttonClear_activate          (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  annotate_clear_screen (); 
}


/* Push color selector button */
G_MODULE_EXPORT void
on_buttonColor_activate	         (GtkToggleToolButton   *toolbutton,
			          gpointer         func_data)
{
  if (!gtk_toggle_tool_button_get_active(toolbutton)) return;
  BarData *bar_data = (BarData*) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  bar_data->pencil = TRUE;
  gchar* new_color = start_color_selector_dialog(GTK_TOOL_BUTTON(toolbutton), GTK_WINDOW(get_bar_window()), bar_data->color);

  /* if it is a valid color */
  if (new_color)
    { 
      set_color(bar_data, new_color);
      g_free(new_color);
    }

  bar_data->grab = grab_value;
  start_tool(bar_data);
}


/* Push blue color button */
G_MODULE_EXPORT void
on_colorBlue_activate            (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_color(bar_data, BLUE);
}


/* Push red color button */
G_MODULE_EXPORT void
on_colorRed_activate             (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_color(bar_data, RED);
}


/* Push green color button */
G_MODULE_EXPORT void
on_colorGreen_activate           (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_color(bar_data, GREEN);
}


/* Push yellow color button */
G_MODULE_EXPORT void
on_colorYellow_activate          (GtkToolButton   *toolbutton,
                                  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_color(bar_data, YELLOW);
}


/* Push white color button */
G_MODULE_EXPORT void
on_colorWhite_activate           (GtkToolButton   *toolbutton,
	       			  gpointer         func_data)
{
  BarData *bar_data = (BarData*) func_data;
  set_color(bar_data, WHITE);
}


