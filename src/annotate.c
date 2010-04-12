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

#include <glib.h>
#include <gdk/gdkinput.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <annotate.h>
#include <broken.h>
#include "background.h"
#include <bezier_spline.h>

#include <cairo.h>

#if defined(_WIN32)
	#include <cairo-win32.h>
        #include <gdkwin32.h>
#else
	#include <cairo-xlib.h>
#endif



#define ANNOTATE_MOUSE_EVENTS    ( GDK_PROXIMITY_IN_MASK |      \
				   GDK_PROXIMITY_OUT_MASK |	\
				   GDK_POINTER_MOTION_MASK |	\
				   GDK_BUTTON_PRESS_MASK |      \
				   GDK_BUTTON_RELEASE_MASK      \
				   )

#define ANNOTATE_KEYBOARD_EVENTS ( GDK_KEY_PRESS_MASK	\
				   )



#define g_slist_first(n) g_slist_nth (n,0)



typedef enum
  {
    ANNOTATE_PEN,
    ANNOTATE_ERASER,
  } AnnotatePaintType;


typedef struct
{
  AnnotatePaintType type;
  guint           width;
  gchar*          fg_color;
  gdouble         pressure;
} AnnotatePaintContext;


typedef struct _AnnotateSave
{
  gint xcursor;
  gint ycursor;
  cairo_surface_t *surface;
  struct _AnnotateSave *next;
  struct _AnnotateSave *previous;
} AnnotateSave;


typedef struct
{
  int untogglexpos;
  int untoggleypos;
  int untogglewidth;
  int untoggleheight;
  cairo_t     *cr;
  GtkWidget   *win;
  
  GdkScreen   *screen;
  GdkDisplay  *display;

  GdkPixmap   *shape;
  AnnotateSave *savelist;
 
  AnnotatePaintContext *default_pen;
  AnnotatePaintContext *default_eraser;
  AnnotatePaintContext *cur_context;

  gdouble      lastx;
  gdouble      lasty;
  GSList       *coordlist;

  GdkDevice   *device;
  guint        state;

  guint        width;
  guint        maxwidth;
  guint        height;
  gboolean     painted;
  gboolean     rectify;
  gboolean     roundify;
  guint        arrow;
  gboolean     debug;
  gboolean     is_grabbed;
  gboolean     cursor_hidden;
} AnnotateData;

AnnotateData* data;

GdkPixmap *transparent_pixmap = NULL;

GdkCursor* cursor = NULL;

/* Create a new paint context */
AnnotatePaintContext * annotate_paint_context_new (AnnotatePaintType type,
                                                   guint width)
{
  AnnotatePaintContext *context;
  context = g_malloc (sizeof (AnnotatePaintContext));
  context->type = type;
  context->width = width;
  context->fg_color = NULL;
  return context;
}


/* Get the annotation window */
GtkWindow* get_annotation_window()
{
  if (data)
    {
      return GTK_WINDOW(data->win);
    }
  else
    {
      return NULL;
    }
}


/* Get the cairo context that contains the annotation */
cairo_t* get_annotation_cairo_context()
{
  return data->cr;
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

  g_printerr ("width: %3d, ", context->width);
  g_printerr ("color: %s\n", context->fg_color);
}



/* Free the memory allocated by paint context */
void annotate_paint_context_free (AnnotatePaintContext *context)
{
  g_free (context);
}


/* Add to the list of the painted point the point (x,y) storing the width */
void annotate_coord_list_append (gint x, gint y, gint width)
{
  AnnotateStrokeCoordinate *point;
  point = g_malloc (sizeof (AnnotateStrokeCoordinate));
  point->x = x;
  point->y = y;
  point->width = width;
  data->coordlist = g_slist_append (data->coordlist, point);
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
void annotate_savelist_free ()
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
  annotate_redolist_free ();
}


/* Calculate the direction in radiants */
gfloat annotate_get_arrow_direction (gboolean revert)
{
  GSList *outptr =   data->coordlist;   
  // make the standard deviation 
  outptr = extract_relevant_points(outptr, FALSE);   
  AnnotateStrokeCoordinate* point = NULL;
  AnnotateStrokeCoordinate* oldpoint = NULL;
  if (revert)
    {
      oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 1);
      point = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 0);
    }
  else
    {
      gint lenght = g_slist_length(outptr);
      oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, lenght-2);
      point = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, lenght-1);
    }
  // calculate the tan beetween the last two point 
  gfloat ret = atan2 (point->y -oldpoint->y, point->x-oldpoint->x);
  g_slist_foreach(outptr, (GFunc)g_free, NULL);
  g_slist_free(outptr); 
  return ret;
}


/* Color selector; if eraser than select the transparent color else alloc the right color */ 
void select_color()
{
  if (!data->cr)
    {
       return;
    }
  if (data->cur_context)
    {
      if (!(data->cur_context->type == ANNOTATE_ERASER))
        {
          if (data->debug)
	    { 
	      g_printerr("Select color %s\n", data->cur_context->fg_color);
	    }
          cairo_set_source_color_from_string(data->cr, data->cur_context->fg_color);
        }
      else
        {
          if (data->debug)
	    { 
	      g_printerr("Select transparent color for erase\n");
	    }
          cairo_set_transparent_color(data->cr);
        }
    }
}


/* Add a save point */
void add_save_point ()
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
  cairo_surface_t* source_surface = cairo_get_target(data->cr);
  cairo_t *cr = cairo_create (saved_surface);
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
  cairo_set_line_cap (data->cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(data->cr, CAIRO_LINE_JOIN_ROUND); 
  cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(data->cr, data->cur_context->width);
  select_color();  
}


/* Destroy old cairo context, allocate a new pixmap and configure the new cairo context */
void reset_cairo()
{
  if (data->cr)
    {
      cairo_new_path(data->cr);
    }
  configure_pen_options();  
}


/* Paint the context over the annotation window */
void merge_context(cairo_t * cr, int yoffset)
{
  reset_cairo();
  cairo_new_path(data->cr);
  cairo_surface_t* source_surface = cairo_get_target(cr);
  cairo_set_operator(data->cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface (data->cr,  source_surface, 0, yoffset);
  cairo_paint(data->cr);
  add_save_point();
}


/* Draw the last save point on the window restoring the surface */
void restore_surface()
{
  if (data->cr)
    {
      AnnotateSave* annotate_save = data->savelist;
      cairo_new_path(data->cr);
      cairo_surface_t* saved_surface = annotate_save->surface;
      cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_surface (data->cr, saved_surface, 0, 0);
      cairo_paint(data->cr);
    }
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
          restore_surface(); 
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
          restore_surface(); 
        }
    }
}


/* Hide the annotations */
void annotate_hide_annotation ()
{
  if (data->debug)
    {
      g_printerr("Hide annotations\n");
    }
  gdk_draw_drawable (data->win->window,
                     data->win->style->fg_gc[GTK_WIDGET_STATE (data->win)],
                     transparent_pixmap,
                     0, 0,
                     0, 0,
                     data->width, data->height);
}


/* Show the annotations */
void annotate_show_annotation ()
{
  if (data->debug)
    {
      g_printerr("Show annotations\n");
    }
  restore_surface();
}


/* Release pointer grab */
void annotate_release_pointer_grab()
{
  ungrab_pointer(data->display,data->win);
  gtk_widget_input_shape_combine_mask(data->win, data->shape, 0, 0);  
  gdk_flush ();
}


/* Release the pointer pointer */
void annotate_release_grab ()
{           
  if (data->is_grabbed)
    {
      annotate_release_pointer_grab(); 
      if (data->debug)
	{
	  g_printerr ("Release grab\n");
	}
      data->is_grabbed=FALSE;
      #ifdef _WIN32 
        gtk_widget_input_shape_combine_mask(data->win, NULL, 0, 0);
      #endif
    }
}


/* Select the default eraser tool */
void annotate_select_eraser()
{
  if (data->debug)
    {
      g_printerr("Select eraser\n");
    }
  AnnotatePaintContext *eraser = data->default_eraser;
  data->cur_context = eraser;
}


/* Configure the eraser */
void annotate_configure_eraser(int width)
{
  data->cur_context->width = (width * 2.5);	
}


/* Select the default pen tool */
void annotate_select_pen()
{
  AnnotatePaintContext *pen = data->default_pen;
  data->cur_context = pen;
}


/* Set the cursor patching the xpm with the selected color */
void set_pen_cursor()
{
  gint size=0;
  #ifdef _WIN32
    size = 5;
  #else
    size=12;
  #endif
  

  gint context_width = data->cur_context->width;;
  GdkPixmap *pixmap = gdk_pixmap_new (NULL, size*3 + context_width, size*3 + context_width, 1);
     
  int circle_width = 2; 
  cairo_t *pen_cr = gdk_cairo_create(pixmap);
  clear_cairo_context(pen_cr);
   
  cairo_set_operator(pen_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(pen_cr, circle_width);
  cairo_set_source_color_from_string(pen_cr, data->cur_context->fg_color);
   
  cairo_arc(pen_cr, 5* size/2 + context_width/2, size/2, (size/2)-circle_width, M_PI * 5/4, M_PI/4);
  cairo_arc(pen_cr, size/2 + context_width/2, 5 * size/2, (size/2)-circle_width, M_PI/4, M_PI * 5/4); 
  cairo_fill(pen_cr);
 
  cairo_arc(pen_cr, size/2 + context_width/2 , 5 * size/2, context_width/2, 0, 2 * M_PI);
  cairo_stroke(pen_cr);
   
  GdkColor *background_color_p = rgb_to_gdkcolor("FFFFFF");
  GdkColor *foreground_color_p = rgb_to_gdkcolor(data->cur_context->fg_color);

  if (cursor)
    {
      gdk_cursor_unref(cursor);
    }
   
  cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap, foreground_color_p, background_color_p, size/2 + context_width/2, 5* size/2);
  gdk_window_set_cursor (data->win->window, cursor);
  gdk_flush ();
  g_object_unref (pixmap);
  g_free(foreground_color_p);
  g_free(background_color_p);
  cairo_destroy(pen_cr);
}


/* Set the eraser cursor */
void set_eraser_cursor()
{
  gint size = 0;
  
  #ifdef _WIN32
    size = data->cur_context->width/5;
    if (size==0) 
      {
        size++;
      }
  #else
    size = data->cur_context->width;
  #endif
    GdkPixmap *pixmap = gdk_pixmap_new (NULL, size, size, 1);
    int circle_width = 2; 
    cairo_t *eraser_cr = gdk_cairo_create(pixmap);
    clear_cairo_context(eraser_cr);
    cairo_set_operator(eraser_cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_line_width(eraser_cr, circle_width);
    cairo_set_source_rgba(eraser_cr,0,0,0,1);
  
    cairo_arc(eraser_cr, size/2, size/2, (size/2)-circle_width, 0, 2 * M_PI);
    cairo_stroke(eraser_cr);
  
    GdkColor *background_color_p = rgb_to_gdkcolor("FFFFFF");
    GdkColor *foreground_color_p = rgb_to_gdkcolor("FF0000");
    if (cursor)
      {
        gdk_cursor_unref(cursor);
      }
    cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap, foreground_color_p, background_color_p, size/2, size/2);
    gdk_window_set_cursor (data->win->window, cursor);
    gdk_flush ();
    g_object_unref (pixmap);
    g_free(foreground_color_p);
    g_free(background_color_p);
    cairo_destroy(eraser_cr);
}


/*
 * This is function return if the point (x,y) in inside the ardesia panel area
 * or in a zone where we must unlock the pointer
 */
gboolean in_unlock_area(int x, int y)
{
  int untogglexpos = data->untogglexpos;
  int untoggleypos = data->untoggleypos;
  int untogglewidth = data->untogglewidth;
  int untoggleheight = data->untoggleheight;
  /* rectangle that contains the panel */
  if ((y>=untoggleypos)&&(y<untoggleypos+untoggleheight))
    {
      if ((x>=untogglexpos)&&(x<untogglexpos+untogglewidth))
	{
	  return 1;
	}
    }
  /* top left corner */
  if ((x<5)&&(y<5))
    {
      return 1;
    }
  return 0;
}

/* Acquire pointer grab */
void annotate_acquire_pointer_grab()
{
  grab_pointer(data->win, ANNOTATE_MOUSE_EVENTS);
}


/* Select pen or eraser cursor */
void annotate_select_cursor()
{
  if(data->cur_context && data->cur_context->type == ANNOTATE_ERASER)
    {
      set_eraser_cursor();
    } 
  else
    {
      set_pen_cursor();
    } 
}


/* Grab the cursor */
void annotate_acquire_grab ()
{
  if  (!data->is_grabbed)
    {
      annotate_acquire_pointer_grab();
      annotate_select_cursor();
  
      data->is_grabbed = TRUE;
      if (data->debug)
	{
	  g_printerr("Acquire grab\n");
	}
    }
}


/* Set color */
void annotate_set_color(gchar* color)
{
  data->cur_context->fg_color = color;
}


/* Set width */
void annotate_set_width(guint width)
{
  data->cur_context->width = width;
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

/* Set arrow type */
void annotate_set_arrow(int arrow)
{
  data->arrow = arrow;
}


/* Start to paint */
void annotate_toggle_grab ()
{ 
  annotate_select_pen();
  annotate_acquire_grab ();
}


/* Start to erase */
void annotate_eraser_grab ()
{
  /* Get the with message */
  annotate_select_eraser();
  annotate_configure_eraser(data->cur_context->width);
  annotate_acquire_grab ();
}


/* Destroy cairo context */
void destroy_cairo()
{
  int refcount = cairo_get_reference_count(data->cr);
  int i = 0;
  for  (i=0; i<refcount; i++)
    {
      cairo_destroy(data->cr);
    }
  data->cr = NULL;
}


/* Clear the annotations windows */
void annotate_clear_screen ()
{
  if (data->debug)
    {
      g_printerr("Clear screen\n");
    }
  reset_cairo();
  clear_cairo_context(data->cr);
  clear_cairo_context(data->cr);
  add_save_point();
  data->painted = TRUE;
}


/* Select eraser, pen or other tool for tablet */
void annotate_select_tool (GdkDevice *device, guint state)
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
              data->cur_context = data->default_eraser;
            }
          else
            {
              data->cur_context = data->default_pen;
	    }
        }
    }
  else
    {
      g_printerr("Attempt to select nonexistent device!\n");
      data->cur_context = data->default_pen;
    }

  data->state = state;
  data->device = device;
}


/* Draw line from (x1,y1) to (x2,y2) */
void annotate_draw_line (gint x1, gint y1,
			 gint x2, gint y2, gboolean stroke)
{
  select_color();  
  if (!stroke)
    {
      cairo_line_to(data->cr, x2, y2);
    }
  else
    {
      cairo_line_to(data->cr, x2, y2);
      cairo_stroke(data->cr);
      cairo_move_to(data->cr, x2, y2);
    }
  data->painted = TRUE;
}


/* Draw an arrow using some polygons */
void annotate_draw_arrow (gboolean revert)
{
  if (data->debug)
    {
      g_printerr("Draw arrow: ");
    }
  select_color();  
  gint lenght = g_slist_length(data->coordlist);
  if (lenght<2)
    {
      return;
    }
  gfloat direction = annotate_get_arrow_direction (revert);
  if (data->debug)
    {
      g_printerr("Arrow direction %f\n", direction/M_PI*180);
    }
  int i = 0;
  if (!revert)
    {
      i = lenght-1; 
    }
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coordlist, i);

  int penwidth = data->cur_context->width;
  
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

  cairo_stroke(data->cr); 
  cairo_save(data->cr);

  cairo_set_line_join(data->cr, CAIRO_LINE_JOIN_MITER); 
  cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(data->cr, penwidth);

  cairo_move_to(data->cr, arrowhead2x, arrowhead2y); 
  cairo_line_to(data->cr, arrowhead1x, arrowhead1y);
  cairo_line_to(data->cr, arrowhead0x, arrowhead0y);
  cairo_line_to(data->cr, arrowhead3x, arrowhead3y);

  cairo_close_path(data->cr);
  cairo_fill_preserve(data->cr);
  cairo_stroke(data->cr);
  cairo_restore(data->cr);
 
  if (data->debug)
    {
      g_printerr("with vertex at (x,y)=(%f : %f)\n",  arrowhead0x , arrowhead0y  );
    }
  data->painted = TRUE;
}



/*
 * Event-Handlers to perform the drawing
 */

/* Device touch */
G_MODULE_EXPORT gboolean
proximity_in (GtkWidget *win,
              GdkEventProximity *ev, 
              gpointer user_data)
{
  gint x, y;
  GdkModifierType state;

  gdk_window_get_pointer (data->win->window, &x, &y, &state);
  annotate_select_tool (ev->device, state);
  if (data->debug)
    {
      g_printerr("Proximity in device %s\n", ev->device->name);
    }
  return TRUE;
}


G_MODULE_EXPORT gboolean
proximity_out (GtkWidget *win, 
               GdkEventProximity *ev,
               gpointer user_data)
{
  
  data->cur_context = data->default_pen;

  data->state = 0;
  data->device = NULL;
  if (data->debug)
    {
      g_printerr("Proximity out device %s\n", ev->device->name);
    }
  return FALSE;
}


/* Hide the cursor */
void hide_cursor()
{
    char invisible_cursor_bits[] = { 0x0 };
    GdkBitmap *empty_bitmap;
    GdkColor color = { 0, 0, 0, 0 };
    empty_bitmap = gdk_bitmap_create_from_data (data->win->window,
					      invisible_cursor_bits,
					      1, 1);
    if (cursor)
      {
        gdk_cursor_unref (cursor);
      }
    cursor = gdk_cursor_new_from_pixmap (empty_bitmap, empty_bitmap, &color,
	             			 &color, 0, 0);
    gdk_window_set_cursor(data->win->window, cursor);
    gdk_flush ();
    g_object_unref(empty_bitmap);
    data->cursor_hidden = TRUE; 
}


/* Unhide the cursor */
void unhide_cursor()
{
  if (data->cursor_hidden)
    {
      if(data->cur_context && data->cur_context->type == ANNOTATE_ERASER)
	{
	  set_eraser_cursor();
	} 
      else
	{
	  set_pen_cursor();
	}
      data->cursor_hidden = FALSE;
    }  
}


/* This is called when the button is pushed */
G_MODULE_EXPORT gboolean
paint (GtkWidget *win,
       GdkEventButton *ev, 
       gpointer user_data)
{ 
  if (!ev)
    {
       return FALSE;
    }
  unhide_cursor();  
  annotate_coord_list_free();
  /* only button1 allowed */
  if (!(ev->button==1))
    {
     if (data->debug)
       {    
         g_printerr("Device '%s': Invalid pressure event\n",
		 ev->device->name);
       }
      return FALSE;
    }  

  reset_cairo();
  cairo_move_to(data->cr,ev->x,ev->y);
  if (in_unlock_area(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return FALSE;
    }   
  if (data->debug)
    {    
      g_printerr("Device '%s': Button %i Down at (x,y)=(%.2f : %.2f)\n",
		 ev->device->name, ev->button, ev->x, ev->y);
    }
  return TRUE;
}


/* This shots when the ponter is moving */
G_MODULE_EXPORT gboolean
paintto (GtkWidget *win, 
         GdkEventMotion *ev, 
         gpointer user_data)
{
  if (!ev)
    {
       if (data->debug)
         {    
           g_printerr("Device '%s': Invalid move event\n",
	              ev->device->name);
         }
       return FALSE;
    }
  unhide_cursor();
   
  // This is a workaround some event inside the bar has wrong coords
  int x,y;
  #ifdef _WIN32
    /* get cursor position */
    gdk_display_get_pointer (data->display, NULL, &x, &y, NULL);
  #else
    x = ev->x;
    y = ev->y;   
  #endif

  if (in_unlock_area(x,y))
    /* point is in the ardesia bar */
    {
       if (data->debug)
         {    
           g_printerr("Device '%s': Move on the bar then ungrab\n",
	              ev->device->name);
         }
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return FALSE;
    }

  gdouble pressure = 0;
  GdkModifierType state = (GdkModifierType) ev->state;  
  /* only button1 allowed */
  if (state & GDK_BUTTON1_MASK)
    {
      pressure=0.5;
    }
  else
    {
      /* the button is not pressed */
      return FALSE;
    }
  if (!(ev->device->source == GDK_SOURCE_MOUSE))
    {
      gint width = (CLAMP (pressure * pressure,0,1) *
		    (double) data->cur_context->width);
      if (width!=data->maxwidth)
        {
          data->maxwidth = width; 
          cairo_set_line_width(data->cr, data->maxwidth);
        }
    }
  annotate_draw_line (data->lastx, data->lasty, ev->x, ev->y, TRUE);
   
  annotate_coord_list_append (ev->x, ev->y, data->maxwidth);

  data->lastx = ev->x;
  data->lasty = ev->y;

  return TRUE;
}


/* This draw an ellipse taking the top left edge coordinates the width and the eight of the bounded rectangle */
void cairo_draw_ellipse(gint x, gint y, gint width, gint height)
{
  if (data->debug)
    {
      g_printerr("Draw ellipse\n");
    }
  select_color();  
  cairo_save(data->cr);
  cairo_translate (data->cr, x + width / 2., y + height / 2.);
  cairo_scale (data->cr, width / 2., height / 2.);
  cairo_arc (data->cr, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore(data->cr);
}


/** Draw the point list */
void draw_point_list(GSList* outptr)
{
  if (outptr)
    {
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)outptr->data;
      gint lastx = out_point->x; 
      gint lasty = out_point->y;
      cairo_move_to(data->cr, lastx, lasty);
      while (outptr)
	    {
	      out_point = (AnnotateStrokeCoordinate*)outptr->data;
	      gint curx = out_point->x; 
	      gint cury = out_point->y;
	      // draw line
	      annotate_draw_line (lastx, lasty, curx, cury, FALSE);
	      lastx = curx;
	      lasty = cury;
	      outptr = outptr->next;   
	    }
    }
}


/* Rectify the line or the shape*/
gboolean rectify()
{
  if (data->debug)
    {
      g_printerr("rectify\n");
    }
  add_save_point();
  annotate_undo();
  annotate_redolist_free();
  reset_cairo();
  gboolean close_path = FALSE;
  GSList *outptr = broken(data->coordlist, &close_path, TRUE);   
  annotate_coord_list_free();
  data->coordlist = outptr;
  draw_point_list(outptr);     
  if (close_path)
    {
      cairo_close_path(data->cr);   
    }
  return close_path;
}


/* Roundify the line or the shape */
gboolean roundify()
{
  if (data->debug)
    {
      g_printerr("Roundify\n");
    }
  add_save_point();
  annotate_undo();
  annotate_redolist_free();
  reset_cairo();
  gboolean close_path = FALSE;
  GSList *outptr = broken(data->coordlist, &close_path, FALSE);   
  annotate_coord_list_free();
  data->coordlist = outptr;

  if (close_path)
    {
      // draw outbounded ellipse with rectangle in the outptr list
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)outptr->data;
      gint lastx = out_point->x; 
      gint lasty = out_point->y;
      AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, 2);
      if (point3)
        { 
          gint curx = point3->x; 
          gint cury = point3->y;
	      cairo_draw_ellipse(lastx,lasty, curx-lastx, cury-lasty);          
        }
    }
  else
    {
      spline(data->cr, outptr);
    }
  return close_path;
}


/* fill the last shape if it is a close path */
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
	     draw_point_list(data->coordlist);     
	     cairo_close_path(data->cr);      
	  }
      select_color();
      cairo_fill(data->cr);
      cairo_stroke(data->cr);
      if (data->debug)
        {
          g_printerr("Fill\n");
        }
      add_save_point();
    }
}


/* This shots when the button is realeased */
G_MODULE_EXPORT gboolean
paintend (GtkWidget *win, GdkEventButton *ev, gpointer user_data)
{
  if (data->debug)
    {
      g_printerr("Device '%s': Button %i Up at (x,y)=(%.2f : %.2f)\n",
		 ev->device->name, ev->button, ev->x, ev->y);
    }
  if (!ev)
    {
       return FALSE;
    }
  /* only button1 allowed */
  if (!(ev->button==1))
    {
      return FALSE;
    }  

  if (in_unlock_area(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return FALSE;
    }

  if (!data->coordlist)
    { 
      cairo_arc (data->cr, ev->x, ev->y, data->cur_context->width/2, 0, 2 * M_PI);
      cairo_fill (data->cr);
      cairo_move_to (data->cr, ev->x, ev->y);
      data->lastx = ev->x;
      data->lasty = ev->y;
      annotate_coord_list_append (ev->x, ev->y, data->maxwidth);
      data->painted = TRUE;
    }
  else
    { 
      AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*)data->coordlist->data;
      int tollerance = 15;
 
      if ((abs(ev->x-first_point->x)<tollerance) &&(abs(ev->y-first_point->y)<tollerance))
	{
	  cairo_line_to(data->cr, first_point->x, first_point->y);
	}
    }

  if (data->coordlist)
    {  
      if (!(data->cur_context->type == ANNOTATE_ERASER))
	{
	  gboolean closed_path = FALSE;
	  /* Rectifier code */
	  if ( g_slist_length(data->coordlist)> 3)
	    {
	      if (data->rectify)
		{
		  closed_path = rectify();
		} 
	      else if (data->roundify)
		{
		  closed_path = roundify(); 
		}
	    }
	  if (!closed_path)
	    {
	      /* If is selected an arrowtype the draw the arrow */
	      if (data->arrow>0)
		{
		  /* print arrow at the end of the line */
		  annotate_draw_arrow (FALSE);
		  if (data->arrow==2)
		    {
		      /* print arrow at the beginning of the line */
		      annotate_draw_arrow (TRUE);
		    }
		}
	    }

	}
    } 
  cairo_stroke_preserve(data->cr);
  add_save_point();
  hide_cursor();  
 
  return TRUE;
}


/*
 * Functions for handling various (GTK+)-Events
 */


/* Expose event: this occurs when the windows is show */

G_MODULE_EXPORT gboolean
event_expose (GtkWidget *widget, 
              GdkEventExpose *event, 
              gpointer user_data)
{
  if (data->debug)
    {
      g_printerr("Expose event\n");
    }
  if (!(data->cr))
    {
      data->cr = gdk_cairo_create(data->win->window);
      annotate_clear_screen();
      transparent_pixmap = gdk_pixmap_new (data->win->window, data->width,
					   data->height, -1);
      cairo_t *transparent_cr = gdk_cairo_create(transparent_pixmap);
      clear_cairo_context(transparent_cr);
      cairo_destroy(transparent_cr);
    }
  restore_surface();
  return TRUE;
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

      /* Dont touch devices with two or more axis - GDK apparently
       * gets confused...  */
      if (device->num_axes > 2)
        {
          g_printerr ("Enabling No. %p: \"%s\" (Type: %d)\n",
                      device, device->name, device->source);
          gdk_device_set_mode (device, GDK_MODE_SCREEN);
        }
    }
}

/* Connect the window to the callback signals */
void annotate_connect_signals()
{ 
  g_signal_connect (data->win, "expose_event",
		    G_CALLBACK (event_expose), NULL);
  g_signal_connect (data->win, "button_press_event", 
		    G_CALLBACK(paint), NULL);
  g_signal_connect (data->win, "motion_notify_event",
		    G_CALLBACK (paintto), NULL);
  g_signal_connect (data->win, "button_release_event",
		    G_CALLBACK (paintend), NULL);
  g_signal_connect (data->win, "proximity_in_event",
		    G_CALLBACK (proximity_in), NULL);
  g_signal_connect (data->win, "proximity_out_event",
		    G_CALLBACK (proximity_out), NULL);

}


/* Setup the application */
void setup_app ()
{ 
  data->cr = NULL;
  data->display = gdk_display_get_default ();
  data->screen = gdk_display_get_default_screen (data->display);

  data->width = gdk_screen_get_width (data->screen);
  data->height = gdk_screen_get_height (data->screen);

  data->win = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_usize (GTK_WIDGET (data->win), data->width, data->height);
  if (STICK)
    {
      gtk_window_stick(GTK_WINDOW(data->win));
    }  
  //gtk_window_fullscreen(GTK_WINDOW(data->win)); 

  #ifndef _WIN32 
    gtk_window_set_opacity(GTK_WINDOW(data->win), 1); 
  #else
    // TODO In windows I am not able to use an rgba transparent  
    // windows and then I use a sort of semi transparency
    gtk_window_set_opacity(GTK_WINDOW(data->win), 0.5); 
  #endif  

 
  data->arrow = 0; 
  data->painted = FALSE;
  data->rectify = FALSE;
  data->roundify = FALSE;

  data->coordlist = NULL;
  data->savelist = NULL;
  data->cr = NULL;
  
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN, 15);
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER, 15);

  data->cur_context = data->default_pen;

  data->state = 0;

  setup_input_devices();
  
  gtk_widget_show_all(data->win);
  
  annotate_connect_signals();
  
  gtk_widget_set_app_paintable(data->win, TRUE);
  gtk_widget_set_double_buffered(data->win, FALSE);

  /* SHAPE PIXMAP */
  data->shape = gdk_pixmap_new (NULL, data->width, data->height, 1); 
  cairo_t* shape_cr = gdk_cairo_create(data->shape);
  clear_cairo_context(shape_cr); 
  cairo_destroy(shape_cr);

  gtk_widget_input_shape_combine_mask(data->win, data->shape, 0, 0);
  /* this allow the pointer focus below the transparent window */ 
  #ifdef _WIN32 
     gtk_widget_input_shape_combine_mask(data->win, NULL, 0, 0);
  #endif
}


/* Quit the annotation */
void annotate_quit()
{
  /* ungrab all */
  annotate_release_grab(); 
  /* destroy cairo */
  destroy_cairo();
  g_signal_handlers_disconnect_by_func (data->win,
					G_CALLBACK (event_expose), NULL);
  gtk_widget_destroy(data->win); 
  data->win = NULL;
  /* free all */
  g_object_unref(transparent_pixmap);
  transparent_pixmap = NULL;
  g_object_unref(data->shape);
  data->shape = NULL;
  annotate_coord_list_free();
  annotate_savelist_free();
  g_free(data->default_pen->fg_color);
  g_free(data->default_pen);
  g_free(data->default_eraser);
  g_free (data);
  if (cursor)
    {
      gdk_cursor_unref(cursor);
    }
}


/* Init the annotation */
int annotate_init (int x, int y, int width, int height, gboolean debug, char* backgroundimage)
{
  data = g_malloc (sizeof (AnnotateData));
  data->debug = debug;
  /* Untoggle zone is setted on ardesia zone */
  data->untogglexpos = x;
  data->untoggleypos = y;
  data->untogglewidth = width;
  data->untoggleheight = height;
  data->cursor_hidden = FALSE;
  data->is_grabbed = FALSE;
 
  setup_app ();
  if (backgroundimage)
    {
      change_background_image(backgroundimage);
    }
  return 0;
}
