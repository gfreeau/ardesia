/* 
 * Ardesia-- a program for painting on the screen 
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

#include <annotate.h>
#include <annotate_callbacks.h>
#include <utils.h>
#include <broken.h>
#include <background.h>
#include <bezier_spline.h>

#ifdef _WIN32
  #include <windows_utils.h>
#endif


static AnnotateData* data;


/* Set color */
void annotate_set_color(gchar* color)
{
  data->cur_context->fg_color = color;
}


/* Set rectifier */
void annotate_set_rectifier(gboolean rectify)
{
  data->rectify = rectify;
}


/* Set rounder */
void annotate_set_rounder(gboolean roundify)
{
  data->roundify = roundify;
}


/* Set arrow */
void annotate_set_arrow(gboolean arrow)
{
  data->arrow = arrow;
}


/* Set the line thickness */
void annotate_set_thickness(gdouble thickness)
{
  data->thickness = thickness;
}


/* Get the line thickness */
gdouble annotate_get_thickness()
{
  if (data->cur_context->type != ANNOTATE_ERASER)
    {
       return data->thickness; 
    }
  return data->thickness*2.5; 
}


/* Create a new paint context */
AnnotatePaintContext* annotate_paint_context_new(AnnotatePaintType type)
{
  AnnotatePaintContext *context;
  context = g_malloc (sizeof(AnnotatePaintContext));
  context->type = type;
  context->fg_color = NULL;
  return context;
}
   

/* Print paint context informations */
void annotate_paint_context_print(gchar *name, AnnotatePaintContext *context)
{
  g_printerr ("Tool name: \"%-20s\": ", name);
  switch (context->type)
    {
    case ANNOTATE_PEN:
      g_printerr ("Pen,     "); break;
    case ANNOTATE_ERASER:
      g_printerr ("Eraser,  "); break;
    default:
      g_printerr ("UNKNOWN, "); break;
    }

  g_printerr ("color: %s\n", context->fg_color);
}


/* Add to the list of the painted point the point (x,y) */
void annotate_coord_list_prepend (gdouble x, gdouble y, gint width, gdouble pressure)
{
  AnnotateStrokeCoordinate *point;
  point = g_malloc (sizeof (AnnotateStrokeCoordinate));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  data->coordlist = g_slist_prepend (data->coordlist, point);
}


/* Free the list of the painted point */
void annotate_coord_list_free()
{
  if (data->coordlist)
    {
      g_slist_foreach(data->coordlist, (GFunc)g_free, NULL);
      g_slist_free(data->coordlist);
      data->coordlist = NULL;
    }
}


/* Delete savepoint */
void delete_save_point(AnnotateSavePoint* savepoint)
{
  if (savepoint)
    {
       if (data->debug)
	 { 
	    g_printerr("Remove %s\n", savepoint->filename);
	 }
       if (savepoint->filename)
         {
            g_remove(savepoint->filename);
            g_free(savepoint->filename);
         }
       if (savepoint->surface)
         {
            cairo_surface_destroy(savepoint->surface);
            savepoint->surface = NULL;
         }
       data->savelist = g_slist_remove(data->savelist, savepoint);
       g_free(savepoint);
       savepoint = NULL;
    }
}


/* Free the list of the  savepoint for the redo */
void annotate_redolist_free()
{
  gint i = data->current_save_index;
  GSList *stop_list = g_slist_nth (data->savelist,i);
  
  while (data->savelist!=stop_list)
    {
      AnnotateSavePoint* savepoint = (AnnotateSavePoint*) g_slist_nth_data (data->savelist, 0);
      delete_save_point(savepoint);  
    }
}


/* Free the list of all the savepoint */
void annotate_savelist_free()
{
  while (data->savelist!=NULL)
    {
      AnnotateSavePoint* savepoint = (AnnotateSavePoint*) g_slist_nth_data (data->savelist, 0);
      delete_save_point(savepoint);
    }
}


/* Modify color according to the pressure */
void annotate_modify_color(AnnotateData* data, gdouble pressure)
{
  /* the pressure is greater than 0 */
  if ((!data->annotation_cairo_context)||(!data->cur_context->fg_color))
    {
	   return;
	}
  if (pressure >= 1)
    {
        cairo_set_source_color_from_string(data->annotation_cairo_context, data->cur_context->fg_color);
    }
  if (pressure <= 0.1)
    {
      pressure = 0.1;
    }
  /* pressure value is from 0 to 1; this value modify the RGBA gradient */
  gint r,g,b,a;
  sscanf (data->cur_context->fg_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
  gdouble old_pressure = pressure;
  if (data->coordlist)
    { 
       AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, 0);
       old_pressure = last_point->pressure;      
    }
  /* if you put an highter value you will have more contrast beetween the lighter and darker color depending on pressure */
  gdouble contrast = 96;
  gdouble corrective = (1-( 3 * pressure + old_pressure)/4) * contrast;
  cairo_set_source_rgba (data->annotation_cairo_context, (r + corrective)/255, (g + corrective)/255, (b+corrective)/255, (gdouble) a/255);
}


/* Calculate the direction in radiants */
gfloat annotate_get_arrow_direction()
{
  /* the list must be not null and the lenght might be greater than two */
  GSList *outptr = data->coordlist;   
  gint delta = 2;


  gint tollerance = data->thickness * delta;
  gfloat ret;  

  AnnotateStrokeCoordinate* point = NULL;
  AnnotateStrokeCoordinate* oldpoint = NULL;
  if (g_slist_length(outptr) >= 3)
    {
       /* extract the relevant point with the standard deviation */
       GSList *relevantpoint_list = extract_relevant_points(outptr, FALSE, tollerance);
       oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (relevantpoint_list, 1);
       point = (AnnotateStrokeCoordinate*) g_slist_nth_data (relevantpoint_list, 0);
       /* give the direction using the last two point */
       ret = atan2 (point->y-oldpoint->y, point->x-oldpoint->x);
       /* free the relevant point list */
       g_slist_foreach(relevantpoint_list, (GFunc)g_free, NULL);
       g_slist_free(relevantpoint_list);
    }  
  else
    {
       oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 1);
       point = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 0);
       // calculate the tan beetween the last two point directly
       ret = atan2 (point->y-oldpoint->y, point->x-oldpoint->x);
    }
  return ret;
}


/* Color selector; if eraser than select the transparent color else alloc the right color */ 
void select_color()
{
  if (!data->annotation_cairo_context)
    {
       return;
    }
  if (data->cur_context)
    {
      if (!(data->cur_context->type == ANNOTATE_ERASER))
        {
          // pen or arrow tool
          if (data->debug)
	    { 
	      g_printerr("Select color %s\n", data->cur_context->fg_color);
	    }
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
          if (data->cur_context->fg_color)
            {
              cairo_set_source_color_from_string(data->annotation_cairo_context, data->cur_context->fg_color);
            }
          else
            { 
              if (data->debug)
	        { 
	          g_printerr("Called select color but this is not allocated\n");
                  g_printerr("I put the red one to recover to the problem\n");
	        }
              cairo_set_source_color_from_string(data->annotation_cairo_context, "FF0000FF");
            }
        }
      else
        {
          // it is the eraser tool
          if (data->debug)
	    { 
	      g_printerr("Select cairo clear operator for erase\n");
	    }
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_CLEAR);
        }
    }
}


/* 
 * Add a save point for the undo/redo; 
 * this code must be called at the end of each painting action
 */
void annotate_add_save_point(gboolean cache)
{
  AnnotateSavePoint *savepoint = g_malloc(sizeof(AnnotateSavePoint));
 
  gdouble ellapsed_time = g_timer_elapsed(data->timer, NULL);

  savepoint->filename = g_strdup_printf("%s%sardesia_%f_vellum.png", data->savepoint_dir, G_DIR_SEPARATOR_S, ellapsed_time);

  /* the story about the future is deleted */
  annotate_redolist_free();

  /* add new savepoint */
  data->savelist = g_slist_prepend (data->savelist, savepoint);
  data->current_save_index = 0;
   
  /* Load a surface with the data->annotation_cairo_context content and write the file */
  cairo_surface_t* saved_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gdk_screen_width(), gdk_screen_height()); 
  cairo_surface_t* source_surface = cairo_get_target(data->annotation_cairo_context);
  cairo_t *cr = cairo_create (saved_surface);
  cairo_set_source_surface (cr, source_surface, 0, 0);
  cairo_paint(cr);
  /* the saved_surface contain the savepoint image */  
  
  if (!cache)
    {
       /*  will be create a file in the savepoint folder with format ardesia_ellapsed_time.png */
       cairo_surface_write_to_png (saved_surface, savepoint->filename);
       cairo_surface_destroy(saved_surface); 
       savepoint->surface = NULL;
       if (data->debug)
         { 
            g_printerr("Add save point in file %s\n", savepoint->filename);
         }
    }
  else
    {
       /* store in cache */
       if (data->debug)
         { 
            g_printerr("Store surface in memory %s\n", savepoint->filename);
         }
     
       savepoint->surface = saved_surface;
    }

  cairo_destroy(cr);

}


/* Configure pen option for cairo context */
void configure_pen_options()
{
  if (data->annotation_cairo_context)
    {
       cairo_set_line_cap (data->annotation_cairo_context, CAIRO_LINE_CAP_ROUND);
       cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_ROUND); 
       cairo_set_line_width(data->annotation_cairo_context, annotate_get_thickness());
    }
  select_color();  
}


/* Set a new cairo path with the new options */
void annotate_reset_cairo()
{
  if (data->annotation_cairo_context)
    {
      cairo_new_path(data->annotation_cairo_context);
    }
  configure_pen_options();  
}


/* Paint the context over the annotation window */
void annotate_push_context(cairo_t * cr)
{
  if (data->debug)
    {
      g_printerr("Merge text window in the annotation window\n");
    }

  annotate_reset_cairo(); 
  cairo_surface_t* source_surface = cairo_get_target(cr);

  #ifndef _WIN32
     /*
      * The over operator might put the new layer on the top of the old one 
      * overriding the intersections
      * Seems that this operator does not work on windows
      * in this operating system only the new layer remain after the merge
      *
      */
     cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_OVER);
  #else
     /*
      * @WORKAROUND using CAIRO_OPERATOR_ADD instead of CAIRO_OPERATOR_OVER 
      * I do this to use the text widget in windows 
      * I use the CAIRO_OPERATOR_ADD that put the new layer
      * on the top of the old; this function does not preserve the color of
      * the second layer but modify it respecting the firts layer
      *
      * Seems that the CAIRO_OPERATOR_OVER does not work because in the
      * gtk cairo implementation the ARGB32 format is not supported
      *
      */
     cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_ADD);
  #endif

  cairo_set_source_surface (data->annotation_cairo_context,  source_surface, 0, 0);
  cairo_paint(data->annotation_cairo_context);
  cairo_stroke(data->annotation_cairo_context);
  annotate_add_save_point(FALSE);
}


/* Draw the last save point on the window restoring the surface */
void annotate_restore_surface()
{
  if (data->annotation_cairo_context)
    {
      gint i = data->current_save_index;
      AnnotateSavePoint* savepoint = (AnnotateSavePoint*) g_slist_nth_data (data->savelist, i);
      if (!savepoint)
         { 
            return;
         }
 
      cairo_new_path(data->annotation_cairo_context);
      cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);

      if (savepoint->surface)
         {
           g_printerr("Load surface from memory %s\n", savepoint->filename);
           cairo_set_source_surface (data->annotation_cairo_context, savepoint->surface, 0, 0);
           cairo_paint(data->annotation_cairo_context);
         }
      else if (savepoint->filename)
         {
           if (data->debug)
             {
               g_printerr("Load file %s\n", savepoint->filename);
             }
           // load the file in the annotation surface
           GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (savepoint->filename, NULL);
           if (pixbuf)
             {
               gdk_cairo_set_source_pixbuf(data->annotation_cairo_context, pixbuf, 0.0, 0.0);
               cairo_paint(data->annotation_cairo_context);
               g_object_unref (pixbuf);
             }
         }

    }
}


/* Select the default pen tool */
void annotate_select_pen()
{
  if (data->debug)
    {
      g_printerr("Select pen\n");
    }
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;
  annotate_set_pen_cursor();
}


/* Select the default eraser tool */
void annotate_select_eraser()
{
  if (data->debug)
    {
      g_printerr("Select eraser\n");
    } 
  data->cur_context = data->default_eraser;
  data->old_paint_type = ANNOTATE_ERASER;
  annotate_set_eraser_cursor();
}


#ifdef _WIN32
  
  /* Acquire the grab pointer */
  void annotate_acquire_pointer_grab()
    {
      grab_pointer(data->annotation_window, ANNOTATE_MOUSE_EVENTS);
    }

  /* Release the grab pointer */
  void annotate_release_pointer_grab()
    {
      ungrab_pointer(gdk_display_get_default(), data->annotation_window);
    }

#endif


/* Update the cursor icon */
void update_cursor()
{
  if (!data->annotation_window)
    {
       return;
    }
  #ifdef _WIN32
    annotate_release_pointer_grab();
  #endif

  gdk_window_set_cursor (data->annotation_window->window, data->cursor);

  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
}


/* Unhide the cursor */
void annotate_unhide_cursor()
{
  if (data->is_cursor_hidden)
    {
      update_cursor();
      data->is_cursor_hidden = FALSE;
    }
}


/* Create pixmap and mask for the invisible cursor; this is used to hide the cursor */
void get_invisible_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  *mask =  gdk_pixmap_new (NULL, size, size, 1); 
  cairo_t *invisible_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(invisible_cr) != CAIRO_STATUS_SUCCESS)
    {
       if (data->debug)
         { 
            g_printerr ("Unable to allocate the cairo context to hide the cursor"); 
            annotate_quit(); 
            exit(1);
         }
    }
  cairo_set_source_rgb(invisible_cr, 1, 1, 1);
  cairo_paint(invisible_cr);
  cairo_stroke(invisible_cr);
  cairo_destroy(invisible_cr);

  cairo_t *invisible_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(invisible_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
       if (data->debug)
         { 
           g_printerr ("Unable to allocate the cairo context for the surface to be restored\n"); 
           annotate_quit(); 
           exit(1);
         }
    }
  clear_cairo_context(invisible_shape_cr);
  cairo_stroke(invisible_shape_cr);
  cairo_destroy(invisible_shape_cr); 
}


/* Allocate a invisible cursor that can be used to hide the cursor icon */
void allocate_invisible_cursor()
{
   GdkPixmap *pixmap, *mask;
   get_invisible_pixmaps(1, &pixmap, &mask);
  
   GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
   GdkColor *foreground_color_p = rgba_to_gdkcolor(WHITE);
  
   data->invisible_cursor = gdk_cursor_new_from_pixmap (pixmap, mask,
                                                         foreground_color_p,
                                                         background_color_p, 
                                                         0, 0);
   g_object_unref(pixmap);
   g_object_unref(mask);
   g_free(foreground_color_p);
   g_free(background_color_p);			
}

 
/* Hide the cursor icon */
void annotate_hide_cursor()
{
  gdk_window_set_cursor(data->annotation_window->window, data->invisible_cursor);
  data->is_cursor_hidden = TRUE;
}


/* Create pixmap and mask for the pen cursor */
void get_pen_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint side_lenght = (size*3) + data->thickness;
  *pixmap = gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);
  *mask =  gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);
  gint circle_width = 2; 

  cairo_t *pen_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(pen_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr ("Unable to allocate the pen cursor cairo context"); 
       annotate_quit(); 
       exit(1);
    }

  cairo_set_operator(pen_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgb(pen_cr, 1, 1, 1);
  cairo_paint(pen_cr);
  cairo_stroke(pen_cr);
  cairo_destroy(pen_cr);

  cairo_t *pen_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(pen_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr ("Unable to allocate the pen shape cursor cairo context"); 
       annotate_quit(); 
       exit(1);
    }

  /* Draw the pen cursor by code */
  clear_cairo_context(pen_shape_cr);
  cairo_set_operator(pen_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(pen_shape_cr, circle_width);
  cairo_set_source_rgb(pen_shape_cr, 1, 1, 1);
  cairo_arc(pen_shape_cr, 5* size/2 + data->thickness/2, size/2, 
            (size/2)-circle_width, M_PI * 5/4, M_PI/4);
  cairo_arc(pen_shape_cr, size/2 + data->thickness/2, 5 * size/2,
            (size/2)-circle_width, M_PI/4, M_PI * 5/4); 
  cairo_fill(pen_shape_cr);
  cairo_arc(pen_shape_cr, size/2 + data->thickness/2 , 5 * size/2,
            data->thickness/2, 0, 2 * M_PI);
  cairo_stroke(pen_shape_cr);
  cairo_destroy(pen_shape_cr);
}


/* Disallocate cursor */
void disallocate_cursor()
{
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor = NULL;
    }
}


/* Set the cursor patching the xpm with the selected color */
void annotate_set_pen_cursor()
{
  gint size=12;

  disallocate_cursor();

  GdkPixmap *pixmap, *mask;
  get_pen_pixmaps(size, &pixmap, &mask); 
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);

  if (data->debug)
    {
      g_printerr("Set pen cursor %s\n", data->cur_context->fg_color);
    }  

  
  GdkColor *foreground_color_p = rgba_to_gdkcolor(data->cur_context->fg_color); 
  gint thickness = data->thickness;

  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, 
                                             foreground_color_p, 
                                             background_color_p,
                                             size/2 + thickness/2, 5* size/2);
  
  g_object_unref(pixmap);
  g_object_unref(mask);
  g_free(foreground_color_p);
  g_free(background_color_p); 
  update_cursor();
}


/* Create pixmap and mask for the eraser cursor */
void get_eraser_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint circle_width = 2; 
  *pixmap = gdk_pixmap_new(NULL, size, size, 1);
  *mask =  gdk_pixmap_new(NULL, size, size, 1);
 
  cairo_t *eraser_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(eraser_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr("Unable to allocate the eraser cursor cairo context"); 
       annotate_quit();
       exit(1);
    }

  cairo_set_source_rgb(eraser_cr, 1, 1, 1);
  cairo_paint(eraser_cr);
  cairo_stroke(eraser_cr);
  cairo_destroy(eraser_cr);

  cairo_t *eraser_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(eraser_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr("Unable to allocate the eraser shape cursor cairo context"); 
       annotate_quit(); 
       exit(1);
    }

  /* paint the eraser circle icon by code */
  clear_cairo_context(eraser_shape_cr);
  cairo_set_operator(eraser_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(eraser_shape_cr, circle_width);
  cairo_set_source_rgb(eraser_shape_cr, 1, 1, 1);
  cairo_arc(eraser_shape_cr, size/2, size/2, (size/2)-circle_width, 0, 2 * M_PI);
  cairo_stroke(eraser_shape_cr);
  cairo_destroy(eraser_shape_cr);
}


/* Set the eraser cursor */
void annotate_set_eraser_cursor()
{   
  disallocate_cursor();
  gint size = annotate_get_thickness();

  GdkPixmap *pixmap, *mask;
  get_eraser_pixmaps(size, &pixmap, &mask); 
  
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor(RED);
 
  data->cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                            foreground_color_p, 
                                            background_color_p, 
                                            size/2, size/2);
 
  g_object_unref(pixmap);
  g_object_unref(mask);
  g_free(foreground_color_p);
  g_free(background_color_p);
  update_cursor();
}


/* Take the input mouse focus */
void annotate_acquire_input_grab()
{
  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
 
  /* the focus is mine */
  gtk_widget_grab_focus(data->annotation_window);
 
  #ifndef _WIN32
    /* 
     * LINUX 
     * remove the input shape; all the input event will stay above the window
     */
    /* 
     * MACOSX
     * in mac this will do nothing 
     */
    gtk_widget_input_shape_combine_mask(data->annotation_window, NULL, 0, 0); 
  #endif
}


/* Grab the cursor */
void annotate_acquire_grab()
{
  if  (!data->is_grabbed)
    {
      if (data->debug)
        {
	   g_printerr("Acquire grab\n");
	}
      annotate_acquire_input_grab();
      data->is_grabbed = TRUE;
    }
}


/* Destroy cairo context */
void destroy_cairo()
{
  gint refcount = cairo_get_reference_count(data->annotation_cairo_context);
  
  gint i = 0;
  for  (i=0; i<refcount; i++)
    {
      cairo_destroy(data->annotation_cairo_context);
    }
}


/* Draw line from the last point drawn to (x2,y2) */
void annotate_draw_line(gdouble x2, gdouble y2, gboolean stroke)
{
  if (!stroke)
    {
      cairo_line_to(data->annotation_cairo_context, x2, y2);
    }
  else
    {
      cairo_line_to(data->annotation_cairo_context, x2, y2);
      cairo_stroke(data->annotation_cairo_context);
      cairo_move_to(data->annotation_cairo_context, x2, y2);
    }
}


/* Draw an arrow using some polygons */
void annotate_draw_arrow(gint distance)
{
  gint arrow_minimum_size = data->thickness * 2;
  if (distance < arrow_minimum_size)
    {  
      return;
    }
  if (data->debug)
    {
      g_printerr("Draw arrow: ");
    }
  gint lenght = g_slist_length(data->coordlist);
  if (lenght < 2)
    {
      /* if it has lenght lesser then two then is a point and it has no sense draw the arrow */
      return;
    }
  
  /* lenght >= 2 */
  gfloat direction = annotate_get_arrow_direction();
  if (data->debug)
    {
      g_printerr("Arrow direction %f\n", direction/M_PI*180);
    }
	
  gint i = 0;
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data(data->coordlist, i);

  gint penwidth = data->thickness;
  
  gdouble widthcos = penwidth * cos(direction);
  gdouble widthsin = penwidth * sin(direction);

  /* Vertex of the arrow */
  gdouble arrowhead0x = point->x + widthcos;
  gdouble arrowhead0y = point->y + widthsin;

  /* left point */
  gdouble arrowhead1x = point->x - widthcos + widthsin ;
  gdouble arrowhead1y = point->y -  widthcos - widthsin ;

  /* origin */
  gdouble arrowhead2x = point->x - 0.8 * widthcos ;
  gdouble arrowhead2y = point->y - 0.8 * widthsin ;

  /* right point */
  gdouble arrowhead3x = point->x - widthcos - widthsin ;
  gdouble arrowhead3y = point->y +  widthcos - widthsin ;

  cairo_stroke(data->annotation_cairo_context); 
  cairo_save(data->annotation_cairo_context);

  /* init cairo properties */
  cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_MITER); 
  cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(data->annotation_cairo_context, penwidth);

  /* draw the arrow */
  cairo_move_to(data->annotation_cairo_context, arrowhead2x, arrowhead2y); 
  cairo_line_to(data->annotation_cairo_context, arrowhead1x, arrowhead1y);
  cairo_line_to(data->annotation_cairo_context, arrowhead0x, arrowhead0y);
  cairo_line_to(data->annotation_cairo_context, arrowhead3x, arrowhead3y);

  cairo_close_path(data->annotation_cairo_context);
  cairo_fill_preserve(data->annotation_cairo_context);
  cairo_stroke(data->annotation_cairo_context);
  cairo_restore(data->annotation_cairo_context);
 
  if (data->debug)
    {
      g_printerr("with vertex at (x,y)=(%f : %f)\n",  arrowhead0x , arrowhead0y  );
    }
}


/* This an ellipse taking the top left edge coordinates the width and the eight of the bounded rectangle */
void annotate_draw_ellipse(gint x, gint y, gint width, gint height)
{
  if (data->debug)
    {
      g_printerr("Draw ellipse\n");
    }

  cairo_save(data->annotation_cairo_context);

  /* the ellipse is done as a 360 degree arc translated */
  cairo_translate(data->annotation_cairo_context, x + width / 2., y + height / 2.);
  cairo_scale(data->annotation_cairo_context, width / 2., height / 2.);
  cairo_arc(data->annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore(data->annotation_cairo_context);
}


/* Draw a point in x,y respecting the context */
void annotate_draw_point(gdouble x, gdouble y, gdouble pressure)
{
  /* modify a little bit the color depending on pressure */
  annotate_modify_color(data, pressure); 
  cairo_move_to(data->annotation_cairo_context, x, y);
  cairo_line_to(data->annotation_cairo_context, x, y);
}


/** Draw the point list */
void annotate_draw_point_list(GSList* list)
{
  if (list)
    {
      AnnotateStrokeCoordinate* out_point;
      while (list)
        {
	  out_point = (AnnotateStrokeCoordinate*)list->data;
	  gdouble curx = out_point->x; 
	  gdouble cury = out_point->y;
          annotate_modify_color(data, out_point->pressure); 
	  // draw line beetween the two points
	  annotate_draw_line (curx, cury, FALSE);
	  list = list->next;   
	}
    }
}


/* Draw a curve using a cubic bezier splines passing to the list's coordinate */
void annotate_draw_curve(GSList* list)
{
   gint lenght = g_slist_length(list);
   gint i = 0;  

   if (list)
    {
      for (i=0; i<lenght; i=i+3)
        {

           AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i);
           AnnotateStrokeCoordinate* second_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i+1);

           if (!second_point)
             {
                return;
             }

           AnnotateStrokeCoordinate* third_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i+2);

           if (!third_point)
             {
                /* draw line from first to second point */
                annotate_draw_line(second_point->x, second_point->y, FALSE);
                return;
             } 

           cairo_curve_to(data->annotation_cairo_context,
		        first_point->x, first_point->y,
		        second_point->x, second_point->y,
		        third_point->x, third_point->y);     

	}
    }
}


/* Rectify the line */
void rectify(gboolean closed_path)
{
  gint tollerance = data->thickness;
  GSList *outptr = broken(data->coordlist, closed_path, TRUE, tollerance);

  if (data->debug)
    {
      g_printerr("rectify\n");
    }

  annotate_add_save_point(TRUE);
  annotate_undo();

  annotate_coord_list_free();
  data->coordlist = outptr;

  annotate_draw_point_list(outptr);     
  if (closed_path)
    {
      cairo_close_path(data->annotation_cairo_context);   
    } 
}


/* Roundify the line */
void roundify(gboolean closed_path)
{
  gint tollerance = data->thickness * 2;
  GSList *outptr = extract_relevant_points(data->coordlist, closed_path, tollerance);  
  gint lenght = g_slist_length(outptr);

  /* Delete the last path drawn */
  annotate_add_save_point(TRUE);
  annotate_undo();
     
  annotate_coord_list_free();
  data->coordlist = outptr;
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, lenght/2);
  annotate_modify_color(data, point->pressure); 
  
  if (lenght == 1)
    {
      /* It is a point */ 
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)outptr->data;
      annotate_draw_point(out_point->x, out_point->y, out_point->pressure);
      return;
    }
  if (lenght == 2)
    {
      /* It is a rect line */
      annotate_draw_point_list(outptr);
      return; 
    }

  if (closed_path)
    {
       // It could be an ellipse or a closed curve path  
       GSList* listOutN = extract_outbounded_rectangle(outptr);

       AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)listOutN->data;
       gdouble lastx = out_point->x; 
       gdouble lasty = out_point->y;
       AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (listOutN, 2);

       /* 
        * if in one point the sum of the distance by focus F1 and F2 differer more than the tollerance value
        * the curve line will not be considered an eclipse 
        */
       gdouble tollerance = (fabs(point3->x-lastx)+fabs(point3->y-lasty)) /4;
       if (is_similar_to_an_ellipse(outptr, listOutN, tollerance))
         {
            annotate_draw_ellipse(lastx, lasty, point3->x-lastx, point3->y-lasty);          
         }
       else
         {
            // it is a closed path but it is not an eclipse I use bezier to spline the path
            GSList* splinedList = spline(outptr);
            annotate_coord_list_free();
            data->coordlist = splinedList;
            annotate_draw_curve(splinedList);
         }
       /* disallocate the outbounded rectangle */
       g_slist_foreach(listOutN, (GFunc)g_free, NULL);
       g_slist_free(listOutN);
     }   
  else
    {
       // It's a not closed path than I use bezier to spline the path
       GSList* splinedList = spline(outptr);
       annotate_coord_list_free();
       data->coordlist = splinedList;
       annotate_draw_curve(splinedList);
    }
}


/* Call the geometric shape recognizer */
void annotate_shape_recognize(gboolean closed_path)
{
  /* rectify only if the list is greater that 3 */
  if ( g_slist_length(data->coordlist)>3)
    {
      if (data->rectify)
        {
	  rectify(closed_path);
	} 
       else if (data->roundify)
	{
	  roundify(closed_path); 
	}
    }
}


/*
 * Functions for setting up (parts of) the application
 */


/* Is the device the "Eraser"-Type device */
gboolean is_eraser(GdkDevice *device)
{
  if (strstr(device->name, "raser") || 
      strstr(device->name, "RASER"))
    {
       return TRUE;
    }
  return FALSE;
}


/* Setup input device */
void setup_input_devices()
{
  GList     *devices, *d;

  devices = gdk_display_list_devices(gdk_display_get_default());
  for (d = devices; d; d = d->next)
    {
      GdkDevice *device = (GdkDevice *) d->data;

      /* Guess "Eraser"-Type devices */
      if (is_eraser(device))
        {
	  gdk_device_set_source(device, GDK_SOURCE_ERASER);
        }

      /* Dont touch devices with lesser than two axis */
      if (device->num_axes >= 2)
        {
           if (device->source != GDK_SOURCE_MOUSE)
           { 
              g_printerr("Enabled Device. %p: \"%s\" (Type: %d)\n",
                      device, device->name, device->source);
              gdk_device_set_mode(device, GDK_MODE_SCREEN);
           }
        }
    }
}


/* Select eraser, pen or other tool for tablet */
void annotate_select_tool(AnnotateData* data, GdkDevice *device, guint state)
{
  if (device)
    {
        if (device->source == GDK_SOURCE_ERASER)
          {
            annotate_select_eraser();
          }
        else
          {
            annotate_select_pen();
	  }
    }
  else
    {
      g_printerr("Attempt to select nonexistent device!\n");
      data->cur_context = data->default_pen;
    } 
}


/* Get the annotation window */
GtkWidget* get_annotation_window()
{
   return data->annotation_window;
}


/* Get the cairo context that contains the annotation */
cairo_t* get_annotation_cairo_context()
{
  return data->annotation_cairo_context;
}


/* Free the memory allocated by paint context */
void annotate_paint_context_free(AnnotatePaintContext *context)
{
  if (context)
    {
       g_free(context);
       context=NULL;
    }
}


/* Quit the annotation */
void annotate_quit()
{
  g_timer_destroy(data->timer); 

 /* unref gtkbuilder */
  if (data->annotationWindowGtkBuilder)
    {
       g_object_unref (data->annotationWindowGtkBuilder);
    }

  if (data->shape)
    {  
       g_object_unref(data->shape);
    }
	
  /* destroy cairo */  
  destroy_cairo();

  /* destroy cursors */  
  disallocate_cursor();
  
  if (data->invisible_cursor)
    {
       gdk_cursor_unref(data->invisible_cursor);
    }

  /* free all */
  if (data->annotation_window)
    {
       gtk_widget_destroy(data->annotation_window); 
    }

  annotate_coord_list_free();
  annotate_savelist_free();

  if (g_file_test(data->savepoint_dir, G_FILE_TEST_IS_DIR))
    { 
       /* Delete the savepoint folder */
       rmdir_recursive(data->savepoint_dir);
    }

  g_free(data->savepoint_dir);
 
  annotate_paint_context_free(data->default_pen);
  annotate_paint_context_free(data->default_eraser);
  annotate_paint_context_free(data->cur_context);

  g_free(data);

}


/* Release input grab; the input event will be passed below the window */
void annotate_release_input_grab()
{
  gdk_window_set_cursor(data->annotation_window->window, NULL);
  #ifndef _WIN32
    /* 
     * @TODO implement correctly gdk_window_input_shape_combine_mask in the quartz gdkwindow or use an equivalent native function
     * the current implementation in macosx this doesn't do nothing
     */
    /*
     * This allows the mouse event to be passed below the transparent annotation; at the moment this call works only on Linux  
     */
    gdk_window_input_shape_combine_mask (data->annotation_window->window, data->shape, 0, 0);
  #else
    /*
     * @TODO WIN32 implement correctly gdk_window_input_shape_combine_mask in the win32 gdkwindow or use an equivalent native function
     * now in the gtk implementation the gdk_window_input_shape_combine_mask call the gdk_window_shape_combine_mask
     * that is not the desired behaviour
     *
     */
     annotate_release_pointer_grab();
  #endif  
}


/* Release the pointer pointer */
void annotate_release_grab ()
{      
  if (data->is_grabbed)
    {
      if (data->debug)
	{
	  g_printerr("Release grab\n");
	}   
      annotate_release_input_grab();
      gtk_window_present(GTK_WINDOW(get_bar_window()));
      data->is_grabbed=FALSE;
    }
}


/* Fill the last shape */
void annotate_fill()
{
  if (data->debug)
    {
      g_printerr("Fill\n");
    }
  if (data->coordlist)
    {
      if (!(data->roundify)&&(!(data->rectify)))
	{
	   annotate_draw_point_list(data->coordlist);     
	   cairo_close_path(data->annotation_cairo_context);      
	}
      select_color();
      cairo_fill(data->annotation_cairo_context);
      cairo_stroke(data->annotation_cairo_context);
      if (data->debug)
        {
           g_printerr("Fill\n");
        }
      annotate_add_save_point(FALSE);
    }
}


/* Undo reverting to the last save point */
void annotate_undo()
{
  if (data->savelist)
    {
      if (data->current_save_index != g_slist_length(data->savelist)-1)
        {
           if (data->debug)
             {
                g_printerr("Undo\n");
             }
           data->current_save_index = data->current_save_index + 1;
           annotate_restore_surface();
        }
    }
}


/* Redo to the last save point */
void annotate_redo()
{
  if (data->savelist)
    {
     if (data->current_save_index != 0)
        {
           if (data->debug)
             {
                g_printerr("Redo\n");
             }
           data->current_save_index = data->current_save_index - 1;
           annotate_restore_surface();
        }
    }
}


/* Clear the annotations windows */
void annotate_clear_screen()
{
  if (data->debug)
    {
      g_printerr("Clear screen\n");
    }

  annotate_reset_cairo();
  clear_cairo_context(data->annotation_cairo_context);
  annotate_add_save_point(FALSE);
}


/* Create the annotation window */
GtkWidget* create_annotation_window()
{
  GtkWidget* widget = NULL;
  GError* error = NULL;

  /* Initialize the main window */
  data->annotationWindowGtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(data->annotationWindowGtkBuilder, ANNOTATION_UI_FILE, &error);

  if (error)
    {
      g_warning ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
      return widget;
    }  
 
  widget = GTK_WIDGET(gtk_builder_get_object(data->annotationWindowGtkBuilder,"annotationWindow")); 
   
  return widget;
}


/* Setup the application */
void setup_app(GtkWidget* parent)
{
  /* initialize the pen context */
  data->default_pen = annotate_paint_context_new(ANNOTATE_PEN);
  data->default_eraser = annotate_paint_context_new(ANNOTATE_ERASER);
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;
  
  setup_input_devices();
  
  /* create the annotation window */
  data->annotation_window = create_annotation_window();
  
  if (data->annotation_window == NULL)
    {
       g_warning("Unable to create the annotation window");
       return;
    }
   
  gtk_window_set_transient_for(GTK_WINDOW(data->annotation_window), GTK_WINDOW(parent));
  
  gtk_window_set_opacity(GTK_WINDOW(data->annotation_window), 1);
  
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();

  gtk_widget_set_usize(data->annotation_window, width, height);

  /* connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(data->annotationWindowGtkBuilder, (gpointer) data); 
   
  /* Initialize a transparent pixmap with depth 1 to be used as input shape */
  data->shape = gdk_pixmap_new(NULL, width, height, 1); 

  cairo_t *shape_cr = gdk_cairo_create(data->shape);
  if (cairo_status(shape_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr ("Unable to allocate the shape cairo context");
       annotate_quit(); 
       exit(1);
    }
  clear_cairo_context(shape_cr);  
  cairo_destroy(shape_cr);

  /* This put the window in fullscreen generating an exposure */
  gtk_window_fullscreen(GTK_WINDOW(data->annotation_window));
 
  gtk_widget_show_all(data->annotation_window);
  
  #ifdef _WIN32
    /* in the gtk 2.16.6 the gtkbuilder property GtkWindow.double-buffered doesn't exist and then I set this by hands */
    gtk_widget_set_double_buffered(data->annotation_window, FALSE); 
    // I use a layered window that use the black as transparent color
    setLayeredGdkWindowAttributes(data->annotation_window->window, RGB(0,0,0), 0, LWA_COLORKEY );	
  #endif
}


void create_savepoint_dir()
{
  const gchar* tmpdir = g_get_tmp_dir();
  data->savepoint_dir = g_strdup_printf("%s%sardesia", tmpdir, G_DIR_SEPARATOR_S);
  if (g_file_test(data->savepoint_dir, G_FILE_TEST_IS_DIR))
    { 
       /* the folder already exist; I delete it */
       rmdir_recursive(data->savepoint_dir);
    }
  g_mkdir(data->savepoint_dir, 0777);
}


/* Init the annotation */
gint annotate_init(GtkWidget* parent, gboolean debug)
{
  data = g_malloc (sizeof(AnnotateData));
 
  /* init the data structure */ 
  data->annotation_cairo_context = NULL;
  data->coordlist = NULL;
  data->savelist = NULL;
  data->current_save_index = 0;
  data->cursor = NULL;

  data->is_grabbed = FALSE;
  data->arrow = FALSE; 
  data->rectify = FALSE;
  data->roundify = FALSE;

  data->is_cursor_hidden = TRUE;

  data->debug = debug;

  data->timer = g_timer_new();  

  allocate_invisible_cursor();

  create_savepoint_dir();  
  setup_app(parent);

  return 0;
}


