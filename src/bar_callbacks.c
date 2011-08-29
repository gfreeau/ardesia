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
#include <bar_callbacks.h>
#include <bar.h>
#include <annotation_window.h>
#include <background_window.h>
#include <text_window.h>
#include <color_selector.h>
#include <preference_dialog.h>
#include <info_dialog.h>
#include <recorder.h>
#include <saver.h>
#include <pdf_saver.h>


/* Timer used to up-rise the window. */
static gint timer = -1;


/* Try to up-rise the window; 
 * this is used for the window manager 
 * that does not support the stay above directive.
 */
static gboolean
bar_to_top (gpointer data)
{
  if (!GTK_WIDGET_VISIBLE(GTK_WIDGET (data)))
    {
       gtk_window_present (GTK_WINDOW (data));
       gtk_widget_grab_focus (data);
       gdk_window_lower (gtk_widget_get_window(GTK_WIDGET(data)));
    }
  return TRUE;
}

/* Called when close the program. */
static gboolean
quit (BarData *bar_data)
{
  annotate_quit ();
  quit_pdf_saver ();
  stop_recorder ();

  /* Dis-allocate all the BarData structure. */
  if (bar_data)
    {
      if (bar_data->color)
        {
          g_free (bar_data->color);
          bar_data->color = NULL;
        }
    }

  gtk_main_quit ();
  return TRUE;
}


/* Is the toggle tool button specified with name is active? */
static gboolean
is_toggle_tool_button_active(gchar *toggle_tool_button_name)
{
  GObject *g_object = gtk_builder_get_object (bar_gtk_builder, toggle_tool_button_name);
  GtkToggleToolButton *toggle_tool_button = GTK_TOGGLE_TOOL_BUTTON (g_object);
  return gtk_toggle_tool_button_get_active (toggle_tool_button);
}


/* Is the text toggle tool button active? */
static
gboolean is_text_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active("buttonText");
}


/* Is the highlighter toggle tool button active? */
static
gboolean is_highlighter_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active("buttonHighlighter");
}


/* Is the eraser toggle tool button active? */
static
gboolean is_eraser_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active("buttonEraser");
}


/* Is the pointer toggle tool button active? */
static
gboolean is_pointer_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active("buttonPointer");
}


/* Is the pointer toggle tool button active? */
static
gboolean is_arrow_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active("buttonArrow");
}


/* Add alpha channel to build the RGBA string. */
static void
add_alpha (BarData *bar_data)
{
  if (is_highlighter_toggle_tool_button_active ())
    {
      strncpy (&bar_data->color[6], SEMI_OPAQUE_ALPHA, 2);
    }
  else
    {
      strncpy (&bar_data->color[6], OPAQUE_ALPHA, 2);
    }
}


/* Select the pen tool. */
static void
take_pen_tool ()
{
  GObject *pencil_obj = gtk_builder_get_object (bar_gtk_builder, "buttonPencil");
  GtkToggleToolButton *pencil_tool_button = GTK_TOGGLE_TOOL_BUTTON (pencil_obj);

  /* Select the pen as default tool. */
  if (is_eraser_toggle_tool_button_active ())
    {
      GObject *eraser_obj = gtk_builder_get_object (bar_gtk_builder, "buttonEraser");
      GtkToggleToolButton *eraser_tool_button = GTK_TOGGLE_TOOL_BUTTON (eraser_obj);
      gtk_toggle_tool_button_set_active (eraser_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }

  if (is_pointer_toggle_tool_button_active ())
    {
      GObject *pointer_obj = gtk_builder_get_object (bar_gtk_builder, "buttonPointer");
      GtkToggleToolButton *pointer_tool_button = GTK_TOGGLE_TOOL_BUTTON (pointer_obj);
      gtk_toggle_tool_button_set_active (pointer_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }

}


/* Release to lock the mouse */
static void
release_lock(BarData *bar_data)
{
  if (bar_data->grab)
    {
      /* Lock enabled. */
      bar_data->grab = FALSE;
      annotate_release_grab ();

      /* Try to up-rise the window. */
      timer = g_timeout_add (BAR_TO_TOP_TIMEOUT, bar_to_top, get_background_window ());
	  
#ifdef _WIN32 // WIN32
      if (gtk_window_get_opacity (GTK_WINDOW (get_background_window ()))!=0)
	{
	  /* 
	   * @HACK This allow the mouse input go below the window putting
           * the opacity to 0; when will be found a better way to make
           * the window transparent to the the pointer input we might
           * remove the previous hack.
           * @TODO Transparent window to the pointer input in a better way.
	   */
	  gtk_window_set_opacity (GTK_WINDOW (get_background_window ()), 0);
	}
#endif

    }
}


/* Lock the mouse. */
static void
lock (BarData *bar_data)
{
  if (!bar_data->grab)
    {
      // Unlock
      bar_data->grab = TRUE;
	  
      /* delete the old timer */
      if (timer!=-1)
        { 
	  g_source_remove (timer);
	  timer = -1;
        }

#ifdef _WIN32 // WIN32

      /* 
       * @HACK Deny the mouse input to go below the window putting the opacity greater than 0
       * @TODO remove the opacity hack when will be solved the next todo.
       */
      if (gtk_window_get_opacity (GTK_WINDOW (get_background_window ()))==0)
	{
	  gtk_window_set_opacity (GTK_WINDOW (get_background_window ()), BACKGROUND_OPACITY);
	}
#endif
    }
}


/* Set color; this is called each time that the user want change color. */
static void
set_color (BarData *bar_data,
	   gchar *selected_color)
{
  take_pen_tool ();
  lock (bar_data);
  strncpy (bar_data->color, selected_color, 6);
  annotate_set_color (bar_data->color);
}


/* Pass the options to the annotation window. */
static void set_options (BarData *bar_data)
{

  annotate_set_rectifier (bar_data->rectifier);
  
  annotate_set_rounder (bar_data->rounder);
  
  annotate_set_thickness (bar_data->thickness);
  
  annotate_set_arrow (is_arrow_toggle_tool_button_active ());

  if (is_eraser_toggle_tool_button_active ())
    {
      annotate_select_eraser ();
   
    }
  else
    {
      annotate_set_color (bar_data->color);
      annotate_select_pen ();
    }

}


/* Start to paint with the selected tool. */
static void
start_tool (BarData *bar_data)
{
  if (bar_data->grab)
    {

      if (is_text_toggle_tool_button_active ())
        {
	  /* Text button then start the text widget. */
	  annotate_release_grab ();
	  start_text_widget (GTK_WINDOW (get_annotation_window ()),
			     bar_data->color,
			     bar_data->thickness);

	}
      else
	{
          /* Is an other tool for paint or erase. */
          set_options (bar_data);
          annotate_acquire_grab ();
	}

    }
}


/* Windows state event: this occurs when the windows state changes. */
G_MODULE_EXPORT gboolean
on_bar_window_state_event (GtkWidget *widget,
			   GdkEventWindowState *event,
			   gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;

  /* Track the minimized signals */
  if(gdk_window_get_state (gtk_widget_get_window (widget)) & GDK_WINDOW_STATE_ICONIFIED)
    {
      release_lock (bar_data);
    }

  return TRUE;
}


/* Configure events occurs. */
G_MODULE_EXPORT gboolean
on_bar_configure_event (GtkWidget *widget,
			GdkEvent *event,
			gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_options (bar_data);
  return TRUE;
}


/* Called when the main window is destroyed. */
G_MODULE_EXPORT void
on_bar_destroy_event            (GtkWidget *widget,
				 gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  quit (bar_data);
}


/* Called when push the quit button */
G_MODULE_EXPORT gboolean
on_bar_quit                     (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  /* Destroy the background window this will call the destroy of all windows. */
  destroy_background_window ();
  return FALSE;
}


/* Called when push the info button. */
G_MODULE_EXPORT gboolean
on_bar_info                      (GtkToolButton   *toolbutton,
				  gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;

  gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
  /* Start the info dialog. */
  start_info_dialog (toolbutton, GTK_WINDOW (get_bar_window ()));

  bar_data->grab = grab_value;
  start_tool (bar_data);
  return TRUE;
}


/* Called when leave the window. */
G_MODULE_EXPORT gboolean
on_bar_leave_notify_event       (GtkWidget       *widget,
				 GdkEvent        *event,
				 gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  add_alpha (bar_data);
  start_tool (bar_data);
  return TRUE;
}


/* Called when enter the window. */
G_MODULE_EXPORT gboolean
on_bar_enter_notify_event       (GtkWidget       *widget,
				 GdkEvent        *event,
				 gpointer         func_data)
{
  if (is_text_toggle_tool_button_active ())
    {
      stop_text_widget ();
    }

  return TRUE;
}


/* Delete event occurs and then I quit the program. */
G_MODULE_EXPORT gboolean
on_bar_delete_event             (GtkWidget       *widget,
				 GdkEvent        *event,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  quit (bar_data);
  return FALSE;
}


/* Push filler button. */
G_MODULE_EXPORT void
on_bar_filler_activate          (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  annotate_fill ();
}


/* Push pointer button. */
G_MODULE_EXPORT void
on_bar_pointer_activate           (GtkToolButton   *toolbutton,
				   gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  release_lock (bar_data);
}


/* Push arrow button. */
G_MODULE_EXPORT void
on_bar_arrow_activate           (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, bar_data->color);
}


/* Push text button. */
G_MODULE_EXPORT void
on_bar_text_activate            (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
}


/* Push highlighter button. */
G_MODULE_EXPORT void
on_bar_highlighter_activate     (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
}


/* Push mode button. */
G_MODULE_EXPORT void
on_bar_mode_activate            (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  if (!bar_data->rectifier)
    {

      if (!bar_data->rounder)
	{
	  /* Select the rounder mode. */
          GObject *rounder_obj = gtk_builder_get_object (bar_gtk_builder, "rounder");
	  gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (rounder_obj));
	  bar_data->rounder = TRUE;
	  bar_data->rectifier = FALSE;
	}
      else
	{
	  /* Select the rectifier mode. */
	  GObject *rectifier_obj = gtk_builder_get_object (bar_gtk_builder, "rectifier");
	  gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (rectifier_obj));
	  bar_data->rectifier = TRUE;
	  bar_data->rounder = FALSE;
	}

    }
  else
    {
      /* Select the free hand writing mode. */
      GObject *hand_obj = gtk_builder_get_object (bar_gtk_builder, "hand");
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (hand_obj));
      bar_data->rectifier = FALSE;
      bar_data->rounder = FALSE;
    }
}


/* Push thickness button. */
G_MODULE_EXPORT void
on_bar_thick_activate           (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;

  if (bar_data->thickness == MICRO_THICKNESS)
    {
      /* Set the thin icon. */
      GObject *thin_obj = gtk_builder_get_object (bar_gtk_builder, "thin");
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (thin_obj));
      bar_data->thickness = THIN_THICKNESS;
    }
  else if (bar_data->thickness == THIN_THICKNESS)
    {
      /* Set the medium icon. */
      GObject *medium_obj = gtk_builder_get_object (bar_gtk_builder, "medium");
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (medium_obj));
      bar_data->thickness = MEDIUM_THICKNESS;
    }
  else if (bar_data->thickness==MEDIUM_THICKNESS)
    {
      /* Set the thick icon. */
      GObject *thick_obj = gtk_builder_get_object (bar_gtk_builder, "thick");
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (thick_obj));
      bar_data->thickness = THICK_THICKNESS;
    }
  else if (bar_data->thickness==THICK_THICKNESS)
    {
      /* Set the micro icon. */
      GObject *micro_obj = gtk_builder_get_object (bar_gtk_builder, "micro");
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (micro_obj));
      bar_data->thickness = MICRO_THICKNESS;
    }

}


/* Push pencil button. */
G_MODULE_EXPORT void
on_bar_pencil_activate          (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  set_color (bar_data, bar_data->color);
}


/* Push eraser button. */
G_MODULE_EXPORT void
on_bar_eraser_activate          (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
}


/* Push save (screen-shoot) button. */
G_MODULE_EXPORT void
on_bar_screenshot_activate	(GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
  start_save_image_dialog (toolbutton, GTK_WINDOW (get_bar_window ()));
  bar_data->grab = grab_value;
  start_tool (bar_data);
}


/* Add page to pdf. */
G_MODULE_EXPORT void
on_bar_add_pdf_activate	        (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  add_pdf_page (GTK_WINDOW (get_bar_window ()));
  bar_data->grab = grab_value;
  start_tool (bar_data);
}


/* Push recorder button. */
G_MODULE_EXPORT void
on_bar_recorder_activate        (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{ 
  BarData *bar_data = (BarData *) func_data;	
  gboolean grab_value = bar_data->grab;

  /* Release grab. */
  annotate_release_grab ();

  bar_data->grab = FALSE;

  if (!is_recorder_available ())	
    {
      /* Visualize a dialog that informs the user about the missing recorder tool. */
      GObject *recorder_obj = gtk_builder_get_object (bar_gtk_builder, "media-recorder-unavailable");
      gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
      visualize_missing_recorder_program_dialog (GTK_WINDOW (get_bar_window ()));
      /* Put an icon that remember that the tool is not available. */
      gtk_tool_button_set_label_widget (toolbutton, GTK_WIDGET (recorder_obj));
      bar_data->grab = grab_value;
      start_tool (bar_data);		
      return;
    }
	
  if (is_started ())
    {
      if (is_paused ())
        {
          resume_recorder ();
        }
      else
        {
          pause_recorder ();
          /* Set the stop tool-tip. */ 
          gtk_tool_item_set_tooltip_text ( (GtkToolItem *) toolbutton, gettext ("Record"));
          /* Put the record icon. */
          gtk_tool_button_set_stock_id (toolbutton, "gtk-media-record");
        }
    }
  else
    { 
      gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
      /* The recording is not active. */ 
      gboolean status = start_save_video_dialog (toolbutton, GTK_WINDOW (get_bar_window ()));
      if (status)
        {
          /* Set the stop tool-tip. */ 
          gtk_tool_item_set_tooltip_text ( (GtkToolItem *) toolbutton, gettext ("Stop"));
          /* Put the stop icon. */
          gtk_tool_button_set_stock_id (toolbutton, "gtk-media-stop");
        }
    }
  bar_data->grab = grab_value;
  start_tool (bar_data);
}


/* Push preference button. */
G_MODULE_EXPORT void
on_bar_preferences_activate	(GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab = FALSE;
  gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
  start_preference_dialog (GTK_WINDOW (get_bar_window ()));
  bar_data->grab = grab_value;
  start_tool (bar_data);
}


/* Push undo button. */
G_MODULE_EXPORT void
on_bar_undo_activate            (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  annotate_undo ();
}


/* Push redo button. */
G_MODULE_EXPORT void
on_bar_redo_activate            (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  annotate_redo ();
}


/* Push clear button. */
G_MODULE_EXPORT void
on_bar_clear_activate           (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  annotate_clear_screen (); 
}


/* Push colour selector button. */
G_MODULE_EXPORT void
on_bar_color_activate	        (GtkToggleToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  gchar* new_color = "";

  if (!gtk_toggle_tool_button_get_active (toolbutton))
    {
      return;
    }

  bar_data->grab = FALSE;
  gdk_window_set_cursor (get_annotation_window ()->window, (GdkCursor *) NULL);
  new_color = start_color_selector_dialog (GTK_TOOL_BUTTON (toolbutton),
					   GTK_WINDOW (get_bar_window ()),
					   bar_data->color);

  if (new_color)  // if it is a valid colour
    { 
      set_color (bar_data, new_color);
      g_free (new_color);
    }

  bar_data->grab = grab_value;
  start_tool (bar_data);
}


/* Push blue colour button. */
G_MODULE_EXPORT void
on_bar_blue_activate            (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, BLUE);
}


/* Push red colour button. */
G_MODULE_EXPORT void
on_bar_red_activate             (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, RED);
}


/* Push green colour button. */
G_MODULE_EXPORT void
on_bar_green_activate           (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, GREEN);
}


/* Push yellow colour button. */
G_MODULE_EXPORT void
on_bar_yellow_activate          (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, YELLOW);
}


/* Push white colour button. */
G_MODULE_EXPORT void
on_bar_white_activate           (GtkToolButton   *toolbutton,
				 gpointer         func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, WHITE);
}


