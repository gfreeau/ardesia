/* 
 * Ardesia-- a program for painting on the screen 
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * Some parts of this file are been copied from gromit
 * Copyright (C) 2000 Simon Budig <Simon.Budig@unix-ag.org>
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


/* Set rectifier */
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
void annotate_set_thickness(double thickness)
{
  data->thickness = thickness;
}


/* Get the line thickness */
double annotate_get_thickness()
{
  if (data->cur_context->type != ANNOTATE_ERASER)
    {
       return data->thickness; 
    }
  return data->thickness*2.5; 
}


/* Create a new paint context */
AnnotatePaintContext * annotate_paint_context_new (AnnotatePaintType type)
{
  AnnotatePaintContext *context;
  context = g_malloc (sizeof (AnnotatePaintContext));
  context->type = type;
  context->fg_color = NULL;
  return context;
}


/* Print paint context informations */
void annotate_paint_context_print (gchar *name, AnnotatePaintContext *context)
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


/* Free the memory allocated by paint context */
void annotate_paint_context_free (AnnotatePaintContext *context)
{
  g_free (context);
}


/* Add to the list of the painted point the point (x,y) storing the width */
void annotate_coord_list_prepend (gint x, gint y, gint width, gdouble pressure)
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
void annotate_coord_list_free ()
{
  if (data->coordlist)
    {
      GSList *ptr = data->coordlist;
      g_slist_foreach(ptr, (GFunc)g_free, NULL);
      g_slist_free(ptr);
      data->coordlist = NULL;
    }
}


/* Free the list of the  savepoint for the redo */
void annotate_redolist_free ()
{
  if (data->savelist)
    {
      if (data->savelist->next)
        {
          AnnotateSave* annotate_save = data->savelist->next;
          /* delete all the savepoint after the current pointed */
          while(annotate_save)
            {
              if (annotate_save->next)
                {
                  annotate_save =  annotate_save->next; 
                  if (annotate_save->previous)
                    {
                      if (annotate_save->previous->surface)
                        {
                          cairo_surface_destroy(annotate_save->previous->surface);
                        } 
                      g_free(annotate_save->previous);
                    }
                }
              else
                {
                  if (annotate_save->surface)
                    {
                      cairo_surface_destroy(annotate_save->surface);
                    } 
                  g_free(annotate_save);
                  break;
                }
            }
          data->savelist->next=NULL;
        }
    }
}


/* Free the list of the savepoint  */
void annotate_savelist_free()
{
  AnnotateSave* annotate_save = data->savelist;

  while (annotate_save)
    {
      if (annotate_save->previous)
	{
	  annotate_save =  annotate_save->previous;
	}
      else
	{
	  break;
	} 
    }
  /* annotate_save point to the first save point */
  data->savelist = annotate_save;
  annotate_redolist_free();
}


/* Modify color according to the pressure */
void modify_color(AnnotateData* data, gdouble pressure)
{
      /* pressure is value from 0 to 1 */
      if ((pressure>0)&&(pressure<1))
        {
          /* this is a number from 0 to 1 to subtract to the RGBA */
          int r,g,b,a;
          sscanf (data->cur_context->fg_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
          gdouble old_pressure = pressure;
          if (data->coordlist)
            { 
               AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, 0);
               old_pressure = last_point->pressure;      
            }
          gdouble contrast = 96;
          gdouble corrective = (1-( 3 * pressure + old_pressure)/4) * contrast;
          cairo_set_source_rgba (data->annotation_cairo_context, (r + corrective)/255, (g + corrective)/255, (b+corrective)/255, (double) a/255);
        }
}


/* Calculate the direction in radiants */
gfloat annotate_get_arrow_direction()
{
  GSList *outptr = data->coordlist;   
  int delta = 2;

  // make the standard deviation 
  int tollerance = data->thickness * delta;
  
  outptr = extract_relevant_points(outptr, FALSE, tollerance);   
  AnnotateStrokeCoordinate* point = NULL;
  AnnotateStrokeCoordinate* oldpoint = NULL;
  oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 1);
  point = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 0);
  // calculate the tan beetween the last two point 
  gfloat ret = atan2 (point->y-oldpoint->y, point->x-oldpoint->x);
  g_slist_foreach(outptr, (GFunc)g_free, NULL);
  g_slist_free(outptr); 
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


/* Add a save point for the undo/redo */
void add_save_point()
{
  AnnotateSave *save = g_malloc(sizeof(AnnotateSave));
  save->previous  = NULL;
  save->next  = NULL;
  save->surface  = NULL;

  if (data->savelist != NULL)
    {
      annotate_redolist_free ();
      data->savelist->next=save;
      save->previous=data->savelist;
    }
  data->savelist=save;

  AnnotateSave* annotate_save = data->savelist;
  if (!(annotate_save->surface))
    {
      annotate_save->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height); 
    }

  cairo_surface_t* saved_surface = annotate_save->surface;
  cairo_surface_t* source_surface = cairo_get_target(data->annotation_cairo_context);
  cairo_t *cr = cairo_create (saved_surface);

  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    {
       if (data->debug)
         { 
           g_printerr ("Unable to allocate the cairo context for the surface to be restored\n"); 
           annotate_quit(); 
           exit(1);
         }
    }

  cairo_set_source_surface (cr, source_surface, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);

  if (data->debug)
    { 
      g_printerr("Add save point\n");
    }
}


/* Configure pen option for cairo context */
void configure_pen_options()
{
  cairo_set_line_cap (data->annotation_cairo_context, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_ROUND); 
  cairo_set_line_width(data->annotation_cairo_context, annotate_get_thickness());
  select_color();  
}


/* Set a new cairo path with the new options */
void reset_cairo()
{
  if (data->annotation_cairo_context)
    {
      cairo_new_path(data->annotation_cairo_context);
    }
  configure_pen_options();  
}


/* Paint the context over the annotation window */
void merge_context(cairo_t * cr)
{
  if (data->debug)
    {
      g_printerr("Merge text window in the annotation window\n");
    }
  reset_cairo();
 
  cairo_surface_t* source_surface = cairo_get_target(cr);
  #ifndef _WIN32
     /*
      * The over operator might put the new layer on the top of the old one 
      * overriding the intersections
      * Seems that this operator does not work on windows
      * in this operating system only the new layer remain after the merge
      */
     cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_OVER);
  #else
     /*
      * @workaround to use it in windows I use the add that put the new layer
      * on the top of the old one but it does not preserve the color of
      * the second layer but modify it respecting the firts layer
      * Seems that the CAIRO_OPERATOR_OVER does not work because in the
      * gtk cairo implementation the ARGB32 format is not supported
      */
     cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_ADD);
  #endif
  cairo_set_source_surface (data->annotation_cairo_context,  source_surface, 0, 0);
  cairo_paint(data->annotation_cairo_context);
  cairo_stroke(data->annotation_cairo_context);

  add_save_point();
}


/* Draw the last save point on the window restoring the surface */
void annotate_restore_surface()
{
  if (data->annotation_cairo_context)
    {
      AnnotateSave* annotate_save = data->savelist;
      cairo_new_path(data->annotation_cairo_context);
      cairo_surface_t* saved_surface = annotate_save->surface;
      cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_surface (data->annotation_cairo_context, saved_surface, 0, 0);
      cairo_paint(data->annotation_cairo_context);
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
      ungrab_pointer(data->display, data->annotation_window);
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
  gdk_display_sync(data->display);

  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
}


/* Unhide the cursor */
void unhide_cursor()
{
  if (data->is_cursor_hidden)
    {
      update_cursor();
      data->is_cursor_hidden = FALSE;
    }
}


/* Create pixmap and mask for the invisible cursor */
void get_invisible_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
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


/* Allocate a invisible cursor that can be used to hide the pointer */
void allocate_invisible_cursor()
{
   GdkPixmap *pixmap, *mask;
   get_invisible_pixmaps(1, &pixmap, &mask);
  
   GdkColor *background_color_p = rgb_to_gdkcolor(BLACK);
   GdkColor *foreground_color_p = rgb_to_gdkcolor(WHITE);
  
   data->invisible_cursor = gdk_cursor_new_from_pixmap (pixmap, mask,
                                                         foreground_color_p,
                                                         background_color_p, 
                                                         0, 0);
   g_object_unref(pixmap);
   g_object_unref(mask);
   g_free(foreground_color_p);
   g_free(background_color_p);			
}

 
/* Hide the cursor */
void hide_cursor()
{
  gdk_window_set_cursor(data->annotation_window->window, data->invisible_cursor);
  gdk_display_sync(data->display);
  data->is_cursor_hidden = TRUE;
}


/* Create pixmap and mask for the pen cursor */
void get_pen_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint side_lenght = (size*3) + data->thickness;
  *pixmap = gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);
  *mask =  gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);
  int circle_width = 2; 

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


/* Set the cursor patching the xpm with the selected color */
void annotate_set_pen_cursor()
{
  gint size=12;

  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }

  GdkPixmap *pixmap, *mask;
  get_pen_pixmaps(size, &pixmap, &mask); 
  GdkColor *background_color_p = rgb_to_gdkcolor(BLACK);

  if (data->debug)
    {
      g_printerr("Set pen cursor %s\n", data->cur_context->fg_color);
    }  

  
  GdkColor *foreground_color_p = rgb_to_gdkcolor(data->cur_context->fg_color); 
  gint thickness = data->thickness;

  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, 
                                             foreground_color_p, 
                                             background_color_p,
                                             size/2 + thickness/2, 5* size/2);
  
  g_object_unref (pixmap);
  g_object_unref (mask);
  g_free(foreground_color_p);
  g_free(background_color_p); 
  update_cursor();
}


/* Create pixmap and mask for the eraser cursor */
void get_eraser_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  int circle_width = 2; 
  *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  *mask =  gdk_pixmap_new (NULL, size, size, 1);
 
  cairo_t *eraser_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(eraser_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr ("Unable to allocate the eraser cursor cairo context"); 
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
       g_printerr ("Unable to allocate the eraser shape cursor cairo context"); 
       annotate_quit(); 
       exit(1);
    }

  /* paint the eraser circle by code */
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
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }
  gint size = annotate_get_thickness();

  GdkPixmap *pixmap, *mask;
  get_eraser_pixmaps(size, &pixmap, &mask); 
  
  GdkColor *background_color_p = rgb_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgb_to_gdkcolor(RED);
 
  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask,
                                             foreground_color_p, 
                                             background_color_p, 
                                             size/2, size/2);
 
  g_object_unref (pixmap);
  g_object_unref (mask);
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
void annotate_acquire_grab ()
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
  int refcount = cairo_get_reference_count(data->annotation_cairo_context);
  
  int i = 0;
  for  (i=0; i<refcount; i++)
    {
      cairo_destroy(data->annotation_cairo_context);
    }

  data->annotation_cairo_context = NULL;
}


/* Draw line from the last point drawn to (x2,y2) */
void annotate_draw_line (gint x2, gint y2, gboolean stroke)
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
void annotate_draw_arrow (int distance)
{
  int arrow_minimum_size = data->thickness * 2;
  if (distance < arrow_minimum_size)
    {  
      return;
    }
  if (data->debug)
    {
      g_printerr("Draw arrow: ");
    }
  gint lenght = g_slist_length(data->coordlist);
  if (lenght<2)
    {
      /* if it has lenght 1 then is a point and I don't draw the arrow */
      return;
    }
  
  gfloat direction = annotate_get_arrow_direction ();
  if (data->debug)
    {
      g_printerr("Arrow direction %f\n", direction/M_PI*180);
    }
	
  int i = 0;
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, i);

  int penwidth = data->thickness;
  
  double widthcos = penwidth * cos (direction);
  double widthsin = penwidth * sin (direction);

  /* Vertex of the arrow */
  double arrowhead0x = point->x + widthcos;
  double arrowhead0y = point->y + widthsin;

  /* left point */
  double arrowhead1x = point->x - widthcos + widthsin ;
  double arrowhead1y = point->y -  widthcos - widthsin ;

  /* origin */
  double arrowhead2x = point->x - 0.8 * widthcos ;
  double arrowhead2y = point->y - 0.8 * widthsin ;

  /* right point */
  double arrowhead3x = point->x - widthcos - widthsin ;
  double arrowhead3y = point->y +  widthcos - widthsin ;

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


/* This draw an ellipse taking the top left edge coordinates the width and the eight of the bounded rectangle */
void cairo_draw_ellipse(gint x, gint y, gint width, gint height)
{
  if (data->debug)
    {
      g_printerr("Draw ellipse\n");
    }

  cairo_save(data->annotation_cairo_context);

  /* the ellipse is done as a 360 degree arc translated */
  cairo_translate (data->annotation_cairo_context, x + width / 2., y + height / 2.);
  cairo_scale (data->annotation_cairo_context, width / 2., height / 2.);
  cairo_arc (data->annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore(data->annotation_cairo_context);
}


/* Draw a point in x,y respecting the context */
void annotate_draw_point(int x, int y)
{
  cairo_move_to (data->annotation_cairo_context, x, y);
  cairo_line_to (data->annotation_cairo_context, x, y);
}


/** Draw the point list */
void annotate_draw_point_list(GSList* outptr)
{
  if (outptr)
    {
      AnnotateStrokeCoordinate* out_point;
      gint lastx; 
      gint lasty;
      while (outptr)
        {
	  out_point = (AnnotateStrokeCoordinate*)outptr->data;
	  gint curx = out_point->x; 
	  gint cury = out_point->y;
	  // draw line beetween the two points
	  annotate_draw_line (curx, cury, FALSE);
	  lastx = curx;
	  lasty = cury;
	  outptr = outptr->next;   
	}
    }
}


/* Rectify the line or the shape */
void rectify(gboolean closed_path)
{
  int tollerance = data->thickness;
  GSList *outptr = broken(data->coordlist, closed_path, TRUE, tollerance);

  if (data->debug)
    {
      g_printerr("rectify\n");
    }

  add_save_point();
  annotate_undo();
  annotate_redolist_free();
  reset_cairo();
       
  annotate_coord_list_free();
  data->coordlist = outptr;

  gint lenght = g_slist_length(outptr);
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, lenght/2);
  modify_color(data, point->pressure); 
  annotate_draw_point_list(outptr);     
  if (closed_path)
    {
      cairo_close_path(data->annotation_cairo_context);   
    }
}


/* Roundify the line or the shape */
void roundify(gboolean closed_path)
{
  int tollerance = data->thickness * 2;
  GSList *outptr = broken(data->coordlist, closed_path, FALSE, tollerance);  
  gint lenght = g_slist_length(outptr);

  /* Delete the last path drawn */
  add_save_point();
  annotate_undo();
  annotate_redolist_free();
  reset_cairo();
     
  annotate_coord_list_free();
  data->coordlist = outptr;
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, lenght/2);
  modify_color(data, point->pressure); 
  
  if (lenght==1)
    {
      /* It is a point */ 
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)outptr->data;
      gint lastx = out_point->x; 
      gint lasty = out_point->y;
      annotate_draw_point(lastx, lasty);
      return;
    }
  if (lenght==2)
    {
      /* It is a rect line */
      annotate_draw_point_list(outptr);
      return; 
    }
  if (closed_path)
    {
      // It's an ellipse draw it with the outbounded rectangle in the outptr list
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)outptr->data;
      gint lastx = out_point->x; 
      gint lasty = out_point->y;
      AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 2);
      if (point3)
        { 
          gint curx = point3->x; 
          gint cury = point3->y;
	  cairo_draw_ellipse(lastx, lasty, curx-lastx, cury-lasty);          
        }
    }
  else
    {
      // It's a not closed line use Bezier splines to smooth it
      spline(data->annotation_cairo_context, outptr);
    }
}


/* Call the geometric shape recognizer */
void shape_recognize(gboolean closed_path)
{
  /* Rectifier code only if the list is greater that 3 */
  if ( g_slist_length(data->coordlist)> 3)
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


/* Setup input device */
void setup_input_devices ()
{
  GList     *tmp_list;

  for (tmp_list = gdk_display_list_devices (data->display);
       tmp_list;
       tmp_list = tmp_list->next)
    {
      GdkDevice *device = (GdkDevice *) tmp_list->data;

      /* Guess "Eraser"-Type devices */
      if (strstr (device->name, "raser") ||
          strstr (device->name, "RASER"))
        {
	  gdk_device_set_source (device, GDK_SOURCE_ERASER);
        }

      /* Dont touch devices with lesser than two axis */
      if (device->num_axes >= 2)
        {
          g_printerr ("Enabling No. %p: \"%s\" (Type: %d)\n",
                      device, device->name, device->source);
          gdk_device_set_mode (device, GDK_MODE_SCREEN);
        }
    }
}


/* Select eraser, pen or other tool for tablet; code inherited by gromit */
void annotate_select_tool (AnnotateData* data, GdkDevice *device, guint state)
{
  guint buttons = 0, modifier = 0, len = 0;
  guint req_buttons = 0, req_modifier = 0;
  guint i, j, success = 0;
  AnnotatePaintContext *context = NULL;
  gchar *name;
 
  if (device)
    {
      len = strlen (device->name);
      name = g_strndup (device->name, len + 3);
      
      /* Extract Button/Modifiers from state (see GdkModifierType) */
      req_buttons = (state >> 8) & 31;

      req_modifier = (state >> 1) & 7;
      if (state & GDK_SHIFT_MASK) req_modifier |= 1;

      name [len] = 124;
      name [len+3] = 0;
  
      /*  0, 1, 3, 7, 15, 31 */
      context = NULL;
      i=-1;
      do
        {
          i++;
          buttons = req_buttons & ((1 << i)-1);
          j=-1;
          do
            {
              j++;
              modifier = req_modifier & ((1 << j)-1);
              name [len+1] = buttons + 64;
              name [len+2] = modifier + 48;
            }
          while (j<=3 && req_modifier >= (1 << j));
        }
      while (i<=5 && req_buttons >= (1 << i));

      g_free (name);

      if (!success)
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
    }
  else
    {
      g_printerr("Attempt to select nonexistent device!\n");
      data->cur_context = data->default_pen;
    }
  
  data->device = device;
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


/* Quit the annotation */
void annotate_quit()
{
  gtk_widget_destroy(data->annotation_window);
  /* unref gtkbuilder */
  g_object_unref (data->annotationWindowGtkBuilder);

  if (data->shape_cr)
    {  
      cairo_destroy(data->shape_cr);
    }

  if (data->shape)
    {  
      g_object_unref(data->shape);
    }

  if (data->transparent_cr)
    {
      cairo_destroy(data->transparent_cr);
    }
	
  /* destroy cairo */  
  destroy_cairo();

  /* destroy cursors */  
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }
  
  if (data->invisible_cursor)
    {
      gdk_cursor_unref(data->invisible_cursor);
      data->invisible_cursor=NULL;
    }

  /* free all */
  if (data->annotation_window)
    {
      gtk_widget_destroy(data->annotation_window); 
      data->annotation_window = NULL;
    }
  
  if (data->transparent_pixmap)
    {
      g_object_unref(data->transparent_pixmap);
      data->transparent_pixmap = NULL;
    }

  annotate_coord_list_free();
  annotate_savelist_free();

  g_free(data->default_pen);
  g_free(data->default_eraser);
 
  g_free (data);
}


/* Release input grab; the input event will must listen below the window */
void annotate_release_input_grab()
{
  gdk_window_set_cursor (data->annotation_window->window, NULL);
  gdk_display_sync(data->display);
  #ifndef _WIN32
    /* 
     * @TODO MACOSX 
     * in macosx this will do nothing and then you will not able to click on the desktop,
     * to fix this you must implement the shaping in the quartz gdkwindow or use some other native function
     * to pass the events below the annotation transparent window
     *
     */
    /*
     * LINUX
     * This allows the mouse event to be passed below the transparent annotation  window thanks to the xshape
     * extension  
     */
    gdk_window_input_shape_combine_mask (data->annotation_window->window, data->shape, 0, 0);
  #else
    /*
     * @TODO WIN32
     * in window implementation the  gdk_window_input_shape_combine_mask call the  gdk_window_shape_combine_mask
     * that it is not the desired behaviour
     * to fix this you must implement correctly the shaping in the win32 gdkwindow or use some other native function
     * to pass the events below the annotation transparent window
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
	  g_printerr ("Release grab\n");
	}   
      annotate_release_input_grab();
      gtk_window_present(GTK_WINDOW(get_bar_window()));
      data->is_grabbed=FALSE;
    }
}


/* Fill the last shape if it is a close path */
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
      add_save_point();
    }
}


/* Hide the annotations */
void annotate_hide_annotation ()
{
  if (data->debug)
    {
      g_printerr("Hide annotations\n");
    }
 
  gdk_draw_drawable (data->annotation_window->window,
                     data->annotation_window->style->fg_gc[GTK_WIDGET_STATE (data->annotation_window)],
                     data->transparent_pixmap,
                     0, 0,
                     0, 0,
                     data->width, data->height);
  
  data->are_annotations_hidden = TRUE;
}


/* Show the annotations */
void annotate_show_annotation ()
{
  if (data->debug)
    {
      g_printerr("Show annotations\n");
    }
  data->are_annotations_hidden = FALSE;  
  annotate_restore_surface();
}


/* Undo to the last save point */
void annotate_undo()
{
  if (data->debug)
    {
      g_printerr("Undo\n");
    }
  if (data->savelist)
    {
      if (data->savelist->previous)
        {
          data->savelist = data->savelist->previous;
          annotate_restore_surface(); 
        }
    }
}


/* Redo to the last save point */
void annotate_redo()
{
  if (data->debug)
    {
      g_printerr("Redo\n");
    }
  if (data->savelist)
    {
      if (data->savelist->next)
        {
          data->savelist = data->savelist->next;
          annotate_restore_surface(); 
        }
    }
}


/* Clear the annotations windows */
void annotate_clear_screen ()
{
  if (data->debug)
    {
      g_printerr("Clear screen\n");
    }

  reset_cairo();
  clear_cairo_context(data->annotation_cairo_context);
  add_save_point();
}


GtkWidget* create_annotation_window()
{
  /* Initialize the main window */
  data->annotationWindowGtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(data->annotationWindowGtkBuilder, ANNOTATION_UI_FILE, NULL);
 
  GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(data->annotationWindowGtkBuilder,"annotationWindow")); 
   
  return widget;
}


/* Setup the application */
void setup_app (GtkWidget* parent)
{
  data->annotation_window = create_annotation_window();
   
  gtk_window_set_transient_for(GTK_WINDOW(data->annotation_window), GTK_WINDOW(parent));

  gtk_widget_set_usize(data->annotation_window, data->width, data->height);
  gtk_window_fullscreen(GTK_WINDOW(data->annotation_window)); 

  // initialize pen context
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN);
  
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER);
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;

  setup_input_devices();
  
  /* Initialize a transparent pixmap with depth 1 to be used as input shape */
  data->shape = gdk_pixmap_new (NULL, data->width, data->height, 1); 
  data->shape_cr = gdk_cairo_create(data->shape);
  if (cairo_status(data->shape_cr) != CAIRO_STATUS_SUCCESS)
    {
       g_printerr ("Unable to allocate the shape cairo context");
       annotate_quit(); 
       exit(1);
    }

  clear_cairo_context(data->shape_cr); 
  
  /* This is important; connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(data->annotationWindowGtkBuilder, (gpointer) data);  
 
  #ifdef _WIN32
    // I use a layered window that use the black as transparent color
    setLayeredGdkWindowAttributes(data->annotation_window->window, RGB(0,0,0), 0, LWA_COLORKEY );	
  #endif
}


/* Init the annotation */
int annotate_init (GtkWidget* parent, gboolean debug)
{
  data = g_malloc (sizeof(AnnotateData));
 
  /* init the data structure */ 
  data->annotation_cairo_context = NULL;
  data->coordlist = NULL;
  data->savelist = NULL;
  data->transparent_pixmap = NULL;
  data->transparent_cr = NULL;
  data->cursor = NULL;

  data->is_grabbed = FALSE;
  data->arrow = FALSE; 
  data->rectify = FALSE;
  data->roundify = FALSE;
  data->are_annotations_hidden = FALSE; 

  data->is_cursor_hidden = TRUE;

  data->debug = debug;

  allocate_invisible_cursor();
  
  data->display = gdk_display_get_default();
  data->screen = gdk_display_get_default_screen(data->display);
  data->width = gdk_screen_get_width(data->screen);
  data->height = gdk_screen_get_height (data->screen);
  
  setup_app(parent);

  return 0;
}


