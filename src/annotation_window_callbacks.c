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


/*
 * Functions for handling various (GTK+)-Events.
 */


#include <annotation_window_callbacks.h>
#include <annotation_window.h>
#include <utils.h>
#include <input.h>


/* Return the pressure passing the event. */
static gdouble
get_pressure       (GdkEvent *ev)
{
  gdouble ret_value = 1.0;
  gdouble pressure = ret_value;

  gboolean ret = gdk_event_get_axis (ev, GDK_AXIS_PRESSURE, &pressure);

  if (ret)
    {
      ret_value = pressure;
    }

  return ret_value;
}


/* On configure event. */
G_MODULE_EXPORT gboolean
on_configure       (GtkWidget      *widget,
                    GdkEventExpose *event,
                    gpointer        user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if (data->debug)
    {
      g_printerr("DEBUG: Annotation window get configure event (%d,%d)\n",
                 gtk_widget_get_allocated_width (widget),
                 gtk_widget_get_allocated_height (widget));
    }
	
  GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (widget));
  gint is_fullscreen = state & GDK_WINDOW_STATE_FULLSCREEN;

  if (!is_fullscreen)
    {
      return FALSE;
    }

  initialize_annotation_cairo_context (data);
  
  if (!data->is_grabbed)
    {
      return FALSE;
    }

  /* Postcondition; data->annotation_cairo_context is not NULL. */
  return TRUE;
}


/* On screen changed. */
G_MODULE_EXPORT void
on_screen_changed       (GtkWidget  *widget,
                         GdkScreen  *previous_screen,
                         gpointer    user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if (data->debug)
    {
      g_printerr ("DEBUG: Annotation window get screen-changed event\n");
    }

  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (widget));
  GdkVisual *visual = gdk_screen_get_rgba_visual (screen);
  
  if (visual == NULL)
    {
      visual = gdk_screen_get_system_visual (screen);
    }

  gtk_widget_set_visual (widget, visual);
}


/* Expose event: this occurs when the window is shown. */
G_MODULE_EXPORT gboolean
on_expose          (GtkWidget *widget,
                    cairo_t   *cr,
                    gpointer   user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if (data->debug)
    {
      g_printerr ("DEBUG: Annotation window get expose event\n");
    }

  annotate_restore_surface ();
  return TRUE;
}


/*
 * Event-Handlers to perform the drawing.
 */


/* This is called when the button is pushed. */
G_MODULE_EXPORT gboolean
on_button_press    (GtkWidget      *win,
                    GdkEventButton *ev,
                    gpointer        user_data)
{

  AnnotateData *data = (AnnotateData *) user_data;
  GdkDevice *master = gdk_event_get_device ( (GdkEvent *) ev);
  
  /* Get the data for this device. */
  AnnotateDeviceData *masterdata = g_hash_table_lookup (data->devdatatable, master);
  
  gdouble pressure = 1.0;
  
  if (data->cur_context == data->default_filler)
    {
      return FALSE;
    }
  
  if (!data->is_grabbed)
    {
      return FALSE;
    }
	
  if (!ev)
    {
      g_printerr ("Device '%s': Invalid event; I ungrab all\n",
                  gdk_device_get_name (master));
      annotate_release_grab ();
      return FALSE;
    }
	
  if (data->debug)
    {
      g_printerr ("Device '%s': Button %i Down at (x,y)= (%f : %f)\n",
                  gdk_device_get_name (master),
                  ev->button,
                  ev->x,
                  ev->y);
    }

#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {
      /* The point is inside the ardesia bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }
#endif

  pressure = get_pressure ( (GdkEvent *) ev);

  if (pressure <= 0)
    {
      return FALSE;
    }
	
  annotate_unhide_cursor ();

  initialize_annotation_cairo_context (data);

  annotate_configure_pen_options (data);

  annotate_coord_dev_list_free (masterdata);
  annotate_draw_point (masterdata, ev->x, ev->y, pressure);

  annotate_coord_list_prepend (masterdata,
                               ev->x,
                               ev->y,
                               annotate_get_thickness (),
                               pressure);

  return TRUE;
}


/* This shots when the pointer is moving. */
G_MODULE_EXPORT gboolean
on_motion_notify   (GtkWidget       *win,
                    GdkEventMotion  *ev,
                    gpointer         user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  GdkDevice *master = gdk_event_get_device ( (GdkEvent *) ev);
  GdkDevice *slave = gdk_event_get_source_device ( (GdkEvent *) ev);
  
  /* Get the data for this device. */
  AnnotateDeviceData *masterdata= g_hash_table_lookup (data->devdatatable, master);
  AnnotateDeviceData *slavedata = g_hash_table_lookup (data->devdatatable, slave);

   if (data->cur_context == data->default_filler)
    {
      return FALSE;
    }
    
  if (ev->state != masterdata->state ||
      ev->state != slavedata->state  ||
      masterdata->lastslave != slave)
    {
       annotate_select_tool (data, master, slave, ev->state);
    }

  gdouble selected_width = 0.0;
  gdouble pressure = 1.0; 

  if (!data->is_grabbed)
    {
      return FALSE;
    }

  if (!ev)
    {
      g_printerr ("Device '%s': Invalid event; I ungrab all\n",
                  gdk_device_get_name (master));
      annotate_release_grab ();
      return FALSE;
    }

  if (data->debug)
    {
      g_printerr ("Device '%s': Move at (x,y)= (%f : %f)\n",
                  gdk_device_get_name (master),
                  ev->x,
                  ev->y);
    }
  
#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {

      if (data->debug)
        {
          g_printerr ("Device '%s': Move on the bar then ungrab\n",
                       gdk_device_get_name (master));
        }

      /* The point is inside the ardesia bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }
#endif

  annotate_unhide_cursor ();
  
  /* Only the first 5 buttons allowed. */
  if(!(ev->state & (GDK_BUTTON1_MASK|
                    GDK_BUTTON2_MASK|
                    GDK_BUTTON3_MASK|
                    GDK_BUTTON4_MASK|
                    GDK_BUTTON5_MASK)))
    {
      return TRUE;
    }

  initialize_annotation_cairo_context (data);

  annotate_configure_pen_options (data);
  
  if (data->cur_context->type != ANNOTATE_ERASER)
    {
      pressure = get_pressure ( (GdkEvent *) ev);

      if (pressure <= 0)
        {
          return FALSE;
        }

      /* If the point is already selected and higher pressure then print else jump it. */
      if (masterdata->coord_list)
        {
          AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (masterdata->coord_list, 0);
          gdouble tollerance = annotate_get_thickness ();

          if (get_distance (last_point->x, last_point->y, ev->x, ev->y)<tollerance)
            {
              /* Seems that you are uprising the pen. */
              if (pressure <= last_point->pressure)
                {
                  /* Jump the point you are uprising the hand. */
                  return FALSE;
                }
              else // pressure >= last_point->pressure
                {
                  /* Seems that you are pressing the pen more. */
                  annotate_modify_color (masterdata, data, pressure);
                  annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
                  /* Store the new pressure without allocate a new coordinate. */
                  last_point->pressure = pressure;
                  return TRUE;
                }
            }
          annotate_modify_color (masterdata, data, pressure);
        }
    }
    
  annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
  annotate_coord_list_prepend (masterdata, ev->x, ev->y, selected_width, pressure);

  return TRUE;
}


/* This shots when the button is released. */
G_MODULE_EXPORT gboolean
on_button_release  (GtkWidget       *win,
                    GdkEventButton  *ev,
                    gpointer         user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  
  GdkDevice *master = gdk_event_get_device ( (GdkEvent *) ev);
  
  /* Get the data for this device. */
  AnnotateDeviceData *masterdata= g_hash_table_lookup (data->devdatatable, master);
  
  guint lenght = g_slist_length (masterdata->coord_list);

  if (!data->is_grabbed)
    {
      return FALSE;
    }
	
  if (!ev)
    {
      g_printerr ("Device '%s': Invalid event; I ungrab all\n",
                  gdk_device_get_name (master));
      annotate_release_grab ();
      return FALSE;
    }

  if (data->debug)
    {
      g_printerr ("Device '%s': Button %i Up at (x,y)= (%.2f : %.2f)\n",
                  gdk_device_get_name (master),
                   ev->button, ev->x, ev->y);
    }

#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    /* Point is in the ardesia bar. */
    {
      /* The last point was outside the bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }
  if (data->old_paint_type == ANNOTATE_PEN)
    {
       annotate_select_pen ();
    }
#endif

  if (data->cur_context == data->default_filler)
    {
      annotate_fill (masterdata, data, ev->x, ev->y);
      return TRUE;
    }
    
  initialize_annotation_cairo_context (data);

  if (lenght > 2)
    {
      AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (masterdata->coord_list, lenght-1);
      AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (masterdata->coord_list, 0);

      gdouble distance = get_distance (ev->x, ev->y, first_point->x, first_point->y);

      /* This is the tolerance to force to close the path in a magnetic way. */
      gint score = 3;
      
      /* If is applied some handled drawing mode then the tool is more tollerant. */
      if ((data->rectify || data->roundify))
        {
          score = 6;
        }
        
      gdouble tollerance = annotate_get_thickness () * score;

      gdouble pressure = last_point->pressure;
      annotate_modify_color (masterdata, data, pressure);

      gboolean closed_path = FALSE;

      /* If the distance between two point lesser than tolerance they are the same point for me. */
      if (distance > tollerance)
        {
          /* Different point. */
          annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
          annotate_coord_list_prepend (masterdata, ev->x, ev->y, annotate_get_thickness (), pressure);
        }
      else
        {
          /* Rounded to be the same point. */
          closed_path = TRUE; // this seems to be a closed path
          annotate_draw_line (masterdata, first_point->x, first_point->y, TRUE);
          annotate_coord_list_prepend (masterdata, first_point->x, first_point->y, annotate_get_thickness (), pressure);
        }

      if (data->cur_context->type != ANNOTATE_ERASER)
        {
          annotate_shape_recognize (masterdata, closed_path);

          /* If is selected an arrow type then I draw the arrow. */
          if (data->arrow)
            {
              /* Print arrow at the end of the path. */
              annotate_draw_arrow (masterdata, distance);
            }
        }
    }

  cairo_stroke (data->annotation_cairo_context);

  annotate_add_savepoint ();

  annotate_hide_cursor ();

  return TRUE;
}


/* On device added. */
void on_device_removed  (GdkDeviceManager  *device_manager,
                         GdkDevice         *device,
                         gpointer           user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if(data->debug)
    {
      g_printerr ("DEBUG: device '%s' removed\n", gdk_device_get_name(device));
    }
    
  remove_input_device (device, data);
}


/* On device removed. */
void on_device_added    (GdkDeviceManager  *device_manager,
                         GdkDevice         *device,
                         gpointer           user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if(data->debug)
    {
      g_printerr ("DEBUG: device '%s' added\n", gdk_device_get_name (device));
    }

  add_input_device (device, data);
}

