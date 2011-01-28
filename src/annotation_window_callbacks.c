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

/*
 * Functions for handling various (GTK+)-Events
 */

#include <annotation_window_callbacks.h>
#include <annotation_window.h>
#include <utils.h>



/* Return the pressure passing the event */
static gdouble get_pressure(GdkEvent* ev)
{
  gdouble pressure;
  gboolean ret = gdk_event_get_axis(ev, GDK_AXIS_PRESSURE, &pressure);
  if (ret)
    {
      return pressure;
    }
  return 1.0;
}


/* Expose event: this occurs when the window is shown */
G_MODULE_EXPORT gboolean
event_expose(GtkWidget *widget, 
	     GdkEventExpose *event, 
	     gpointer func_data)
{
  AnnotateData *data = (AnnotateData *) func_data;

  gint is_fullscreen = gdk_window_get_state(widget->window) & GDK_WINDOW_STATE_FULLSCREEN;
  if (!is_fullscreen)
    {
      
      return TRUE;
    }

  if (data->debug)
    {
      g_printerr("Expose event\n");
    }

  if (!(data->annotation_cairo_context))
    {
      /* initialize a transparent window */	  
#ifdef _WIN32
      /* The hdc has depth 32 and the technology is DT_RASDISPLAY */
      HDC hdc = GetDC(GDK_WINDOW_HWND(data->annotation_window->window));
      /* 
       * @TODO Use an HDC that support the ARGB32 format to support the alpha channel and the highlighter
       * In the documentation is written that the now the resulting surface is in RGB24 format
       * 
       */
      cairo_surface_t* surface = cairo_win32_surface_create(hdc);

      data->annotation_cairo_context = cairo_create(surface);
#else
      data->annotation_cairo_context = gdk_cairo_create(data->annotation_window->window);  
#endif 

      if (cairo_status(data->annotation_cairo_context) != CAIRO_STATUS_SUCCESS)
        {
          g_printerr("Failed to allocate the annotation cairo context"); 
          annotate_quit(); 
          exit(EXIT_FAILURE);
        }    
 		
      annotate_clear_screen();
      annotate_acquire_grab();		      
    }

  gdk_cairo_region(data->annotation_cairo_context, event->region); 
  annotate_restore_surface();
  return TRUE;
}


/*
 * Event-Handlers to perform the drawing
 */


/* This is called when the button is pushed */
G_MODULE_EXPORT gboolean
paint(GtkWidget *win,
      GdkEventButton *ev, 
      gpointer func_data)
{ 

  AnnotateData *data = (AnnotateData *) func_data;
 
  if (!ev)
    {
      g_printerr("Device '%s': Invalid event; I ungrab all\n",
		 ev->device->name);
      annotate_release_grab();
      return TRUE;
    }

  if (data->debug)
    {    
      g_printerr("Device '%s': Button %i Down at (x,y)=(%f : %f)\n",
		 ev->device->name, ev->button, ev->x, ev->y);
    }

#ifdef _WIN32
  if (inside_bar_window(ev->x_root, ev->y_root))
    {
      /* the point is inside the ardesia bar then ungrab */
      annotate_release_grab();
      return TRUE;
    }   
#endif

  annotate_coord_list_free();
 
  annotate_unhide_cursor();
 
  /* only button1 allowed */
  if (!(ev->button == 1))
    {
      if (data->debug)
	{    
	  g_printerr("Device '%s': Invalid pressure event\n",
		     ev->device->name);
	}
      return TRUE;
    }  

  annotate_reset_cairo();

  gdouble pressure = 1.0; 
  if ((ev->device->source != GDK_SOURCE_MOUSE) && (data->cur_context->type != ANNOTATE_ERASER))
    {
      pressure = get_pressure((GdkEvent *) ev);
      if (pressure <= 0)
	{
	  return TRUE;
	}
    }

  annotate_draw_point(ev->x, ev->y, pressure);  
 
  annotate_coord_list_prepend(ev->x, ev->y, annotate_get_thickness(), pressure);
  return TRUE;
}


/* This shots when the ponter is moving */
G_MODULE_EXPORT gboolean
paintto(GtkWidget *win, 
        GdkEventMotion *ev, 
        gpointer func_data)
{
  AnnotateData *data = (AnnotateData *) func_data;
 
  if (!ev)
    {
      g_printerr("Device '%s': Invalid event; I ungrab all\n",
		 ev->device->name);
      annotate_release_grab();
      return TRUE;
    }
  
#ifdef _WIN32
  if (inside_bar_window(ev->x_root, ev->y_root))
    {
      if (data->debug)
	{    
	  g_printerr("Device '%s': Move on the bar then ungrab\n",
		     ev->device->name);
	}
      /* the point is inside the ardesia bar then ungrab */
      annotate_release_grab();
      return TRUE;
    }
#endif

  annotate_unhide_cursor();
 
  GdkModifierType state = (GdkModifierType) ev->state;  
  gint selected_width = 0;
  /* only button1 allowed */
  if (!(state & GDK_BUTTON1_MASK))
    {
      /* the button is not pressed */
      return TRUE;
    }

  gdouble pressure = 1.0; 
  if ((ev->device->source != GDK_SOURCE_MOUSE) && (!(data->cur_context->type == ANNOTATE_ERASER)))
    {

      pressure = get_pressure((GdkEvent *) ev);
      if (pressure <= 0)
	{
	  return TRUE;
	}

      /* If the point is already selected and higher pressure then print else jump it */
      if (data->coordlist)
	{
	  AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(data->coordlist, 0);
	  gint tollerance = data->thickness;
	  if (get_distance(last_point->x, last_point->y, ev->x, ev->y)<tollerance)
	    {
	      /* seems that you are uprising the pen */
	      if (pressure < last_point->pressure)
		{
		  /* jump the point you are uprising the hand */
		  return TRUE;
		}
	      else if (pressure == last_point->pressure)
		{
		  /* ignore the point; do nothing */
		  return TRUE;
		}
	      else // pressure >= last_point->pressure
		{
		  /* seems that you are pressing the pen more */
		  annotate_modify_color(data, pressure);
		  annotate_draw_line(ev->x, ev->y, TRUE);
		  /* store the new pressure without allocate a new coord */
		  last_point->pressure = pressure;
		  return TRUE;
		}
	    }
	  annotate_modify_color(data, pressure);
	}
    }

  annotate_draw_line(ev->x, ev->y, TRUE);
   
  annotate_coord_list_prepend(ev->x, ev->y, selected_width, pressure);

  return TRUE;
}


/* This shots when the button is realeased */
G_MODULE_EXPORT gboolean
paintend (GtkWidget *win, GdkEventButton *ev, gpointer func_data)
{
  AnnotateData *data = (AnnotateData *) func_data;
  
  if (!ev)
    {
      g_printerr("Device '%s': Invalid event; I ungrab all\n",
		 ev->device->name);
      annotate_release_grab();
      return TRUE;
    }
  
  if (data->debug)
    {
      g_printerr("Device '%s': Button %i Up at (x,y)=(%.2f : %.2f)\n",
		 ev->device->name, ev->button, ev->x, ev->y);
    }

#ifdef _WIN32
  if (inside_bar_window(ev->x_root, ev->y_root))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab();
      return TRUE;
    }
#endif 
	
  /* only button1 allowed */
  if (!(ev->button == 1))
    {
      return TRUE;
    }  

  gint distance = -1;
  gint lenght = g_slist_length(data->coordlist);

  if (lenght > 2)
    { 
      gint lenght = g_slist_length(data->coordlist);
      AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(data->coordlist, lenght-1);
       
      /* This is the tollerance to force to close the path in a magnetic way */
      gint tollerance = data->thickness * 2;
      distance = get_distance(ev->x, ev->y, first_point->x, first_point->y);
 
      AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, 0);
      gdouble pressure = last_point->pressure;      

      /* If the distance between two point lesser than tollerance they are the same point for me */
      if (distance <= tollerance)
        {
          distance=0;
	  cairo_line_to(data->annotation_cairo_context, first_point->x, first_point->y);
          annotate_coord_list_prepend(first_point->x, first_point->y, data->thickness, pressure);
	}
      else
        {
          cairo_line_to(data->annotation_cairo_context, ev->x, ev->y);
          annotate_coord_list_prepend(ev->x, ev->y, data->thickness, pressure);
        }
   
      if (!(data->cur_context->type == ANNOTATE_ERASER))
        { 
          gboolean closed_path = (distance == 0) ; 
          annotate_shape_recognize(closed_path);
          /* If is selected an arrowtype then I draw the arrow */
          if (data->arrow)
            {
	      /* print arrow at the end of the path */
	      annotate_draw_arrow(distance);
	    }
	}
    }
  cairo_stroke_preserve(data->annotation_cairo_context);
  
  annotate_add_save_point(FALSE);
   
  annotate_hide_cursor();  
 
  return TRUE;
}


/* Device touch */
G_MODULE_EXPORT gboolean
proximity_in(GtkWidget *win,
             GdkEventProximity *ev, 
             gpointer func_data)
{
  /*
   * @TODO this message doesn't arrive on windows; why? 
   * is it a driver problem, gtk or what
   *
   */
  AnnotateData *data = (AnnotateData *) func_data;
  if (data->debug)
    {
      g_printerr("Proximity in device %s\n", ev->device->name);
    }

  if (data->cur_context->type == ANNOTATE_PEN)
    {
      gint x, y;
      GdkModifierType state;
      gdk_window_get_pointer(win->window, &x, &y, &state);
      annotate_select_tool(data, ev->device, state);
      data->old_paint_type = ANNOTATE_PEN; 
    }
  else
    {
      data->old_paint_type = ANNOTATE_ERASER; 
    }

  return TRUE;
}


/* Device lease */
G_MODULE_EXPORT gboolean
proximity_out(GtkWidget *win, 
              GdkEventProximity *ev,
              gpointer func_data)
{

  /*
   * @TODO this message doesn't arrive on windows; why? 
   * is it a driver problem, gtk or what
   *
   */
  AnnotateData *data = (AnnotateData *) func_data;
  if (data->debug)
    {
      g_printerr("Proximity out device %s\n", ev->device->name);
    }

  if (data->old_paint_type == ANNOTATE_PEN)
    {
      annotate_select_pen();
    }

  return FALSE;
}


