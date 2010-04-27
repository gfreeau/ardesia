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
#include <background.h>
#include <bezier_spline.h>

#include <cairo.h>

#ifdef _WIN32
  #include <cairo-win32.h>
  #include <gdkwin32.h>
  #include <winuser.h>  
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


/* Enumeration containing tools */
typedef enum
  {
    ANNOTATE_PEN,
    ANNOTATE_ERASER,
  } AnnotatePaintType;


/* Paint context */
typedef struct
{
  AnnotatePaintType type;
  guint           width;
  gchar*          fg_color;
  gdouble         pressure;
} AnnotatePaintContext;


/* Struct to store the save point */
typedef struct _AnnotateSave
{
  gint xcursor;
  gint ycursor;
  cairo_surface_t *surface;
  struct _AnnotateSave *next;
  struct _AnnotateSave *previous;
} AnnotateSave;


/* Annotation data used by the callsbacks */
typedef struct
{
  GdkScreen   *screen;
  GdkDisplay  *display;

  /* the annotation window */   
  GtkWidget   *win;
  /* cairo context attached to the window */
  cairo_t     *cr;
  
  /* the shape pixmap used as mask */   
  GdkPixmap   *shape;
  /* cairo context attached to the shape pixmap */
  cairo_t     *shape_cr;
   
  /* bar position */
  int untogglexpos;
  int untoggleypos;
  int untogglewidth;
  int untoggleheight;

  /* transparent pixmap */
  GdkPixmap *transparent_pixmap;
  /* transparent cairo context */
  cairo_t *transparent_cr;
 
  /* mouse cursor */ 
  GdkCursor *cursor;
 
  /* List of the savepoint */ 
  AnnotateSave *savelist;

  /* Paint context */ 
  AnnotatePaintContext *default_pen;
  AnnotatePaintContext *default_eraser;
  AnnotatePaintContext *cur_context;

  /* last point inserted */
  gdouble      lastx;
  gdouble      lasty;

  /* list of the coodinates of the last line drawn */
  GSList       *coordlist;

  /* tablet device */
  GdkDevice   *device;
  guint        state;

  /* width of the screen */
  guint        width;
  guint        height;
 
  /* max thickness of the line depending on pressure */ 
  guint        maxthckness;
 
  /* post draw operation */ 
  gboolean     rectify;
  gboolean     roundify;
  gboolean     arrow;
  
  /* is the cursor grabbed */
  gboolean     is_grabbed;
  
  /* is the cursor hidden */
  gboolean     cursor_hidden;
  
  /* is the debug enabled */
  gboolean     debug;
} AnnotateData;

static AnnotateData* data;


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
GtkWidget* get_annotation_window()
{
  if (data)
    {
      return data->win;
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


/* Calculate the direction in radiants */
gfloat annotate_get_arrow_direction()
{
  GSList *outptr =   data->coordlist;   
  // make the standard deviation 
  outptr = extract_relevant_points(outptr, FALSE);   
  AnnotateStrokeCoordinate* point = NULL;
  AnnotateStrokeCoordinate* oldpoint = NULL;
  gint lenght = g_slist_length(outptr);
  oldpoint = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, lenght-2);
  point = (AnnotateStrokeCoordinate*) g_slist_nth_data (outptr, lenght-1);
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
          cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
          if (data->cur_context->fg_color)
            {
              cairo_set_source_color_from_string(data->cr, data->cur_context->fg_color);
            }
          else
            {
              if (data->debug)
	            { 
	              g_printerr("Called select color but this is not allocated\n");
                  g_printerr("I put the red one to recover to the problem\n");
	            }
              cairo_set_source_color_from_string(data->cr, "FF0000FF");
            }
        }
      else
        {
          if (data->debug)
	    { 
	      g_printerr("Select cairo clear operator for erase\n");
	    }
          cairo_set_operator(data->cr, CAIRO_OPERATOR_CLEAR);
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
void merge_context(cairo_t * cr)
{
  reset_cairo();
 
  //This seems broken on windows
  cairo_surface_t* source_surface = cairo_get_target(cr);
  cairo_set_operator(data->cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface (data->cr,  source_surface, 0, 0);
  cairo_paint(data->cr);
  cairo_stroke(data->cr);
  
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
                     data->transparent_pixmap,
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

  /* This allows the mouse event to be passed to the window below when ungrab */
  #ifndef _WIN32
    gdk_window_input_shape_combine_mask (data->win->window, data->shape, 0, 0);  
  #else
    /* This apply a shape in the ardesia bar; in win32 if the pointer is above the bar the events will passed to the window below */
    cairo_set_source_rgb(data->shape_cr, 1, 1, 1);
    cairo_paint(data->shape_cr);
    cairo_stroke(data->shape_cr);
    cairo_set_source_rgb(data->shape_cr, 0, 0, 0);
    cairo_rectangle(data->shape_cr, data->untogglexpos, data->untoggleypos, data->untogglewidth, data->untoggleheight);
    cairo_fill(data->shape_cr);
    cairo_stroke(data->shape_cr);

    gdk_window_shape_combine_mask (data->win->window, data->shape, 0, 0);  
  #endif

  gdk_flush();
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
      annotate_release_pointer_grab(); 
      data->is_grabbed=FALSE;
    }
  gtk_window_present(GTK_WINDOW(get_bar_window()));
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


/* Acquire pointer grab */
void annotate_acquire_pointer_grab()
{
  grab_pointer(data->win, ANNOTATE_MOUSE_EVENTS);
}


/* Create pixmap and mask for the invisible cursor */
void get_invisible_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  *mask =  gdk_pixmap_new (NULL, size, size, 1); 
  cairo_t *eraser_cr = gdk_cairo_create(*pixmap);
  cairo_set_source_rgb(eraser_cr, 1, 1, 1);
  cairo_paint(eraser_cr);
  cairo_stroke(eraser_cr);
  cairo_destroy(eraser_cr);

  cairo_t *eraser_shape_cr = gdk_cairo_create(*mask);
  clear_cairo_context(eraser_shape_cr);
  cairo_stroke(eraser_shape_cr);
  cairo_destroy(eraser_shape_cr); 
}


/* Hide the cursor */
void hide_cursor()
{
  if (data->cursor)
    {
      gdk_cursor_unref (data->cursor);
      data->cursor=NULL;
    }
  
  #ifdef _WIN32
    annotate_release_pointer_grab();
  #endif
  
  int size = 1;
  
  GdkPixmap *pixmap, *mask;
  get_invisible_pixmaps(size, &pixmap, &mask);
  
  GdkColor *background_color_p = rgb_to_gdkcolor("000000");
  GdkColor *foreground_color_p = rgb_to_gdkcolor("FFFFFF");
  
  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, foreground_color_p, background_color_p, size/2, size/2);    
  
  gdk_window_set_cursor(data->win->window, data->cursor);
  gdk_display_sync(gdk_display_get_default());
  
  g_object_unref (pixmap);
  g_object_unref (mask);
  g_free(foreground_color_p);
  g_free(background_color_p);
  
  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
  data->cursor_hidden = TRUE; 
}


/* Create pixmap and mask for the pen cursor */
void get_pen_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint context_width = data->cur_context->width;;
  *pixmap = gdk_pixmap_new (NULL, size*3 + context_width, size*3 + context_width, 1);
  *mask =  gdk_pixmap_new (NULL, size*3 + context_width, size*3 + context_width, 1);
  int circle_width = 2; 

  cairo_t *pen_cr = gdk_cairo_create(*pixmap);
  cairo_set_operator(pen_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgb(pen_cr, 1, 1, 1);
  cairo_paint(pen_cr);
  cairo_stroke(pen_cr);
  cairo_destroy(pen_cr);

  cairo_t *pen_shape_cr = gdk_cairo_create(*mask);
  clear_cairo_context(pen_shape_cr);
  cairo_set_operator(pen_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(pen_shape_cr, circle_width);
  cairo_set_source_rgb(pen_shape_cr, 1, 1, 1);
  cairo_arc(pen_shape_cr, 5* size/2 + context_width/2, size/2, (size/2)-circle_width, M_PI * 5/4, M_PI/4);
  cairo_arc(pen_shape_cr, size/2 + context_width/2, 5 * size/2, (size/2)-circle_width, M_PI/4, M_PI * 5/4); 
  cairo_fill(pen_shape_cr);
  cairo_arc(pen_shape_cr, size/2 + context_width/2 , 5 * size/2, context_width/2, 0, 2 * M_PI);
  cairo_stroke(pen_shape_cr);
  cairo_destroy(pen_shape_cr);
}


/* Set the cursor patching the xpm with the selected color */
void set_pen_cursor()
{
  #ifdef _WIN32
    annotate_release_pointer_grab();
  #endif
  gint size=12;
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }
  GdkPixmap *pixmap, *mask;
  get_pen_pixmaps(size, &pixmap, &mask); 
  GdkColor *background_color_p = rgb_to_gdkcolor("000000");
  GdkColor *foreground_color_p = rgb_to_gdkcolor(data->cur_context->fg_color); 
  gint context_width = data->cur_context->width;

  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, foreground_color_p, background_color_p, size/2 + context_width/2, 5* size/2);

  gdk_window_set_cursor (data->win->window, data->cursor);
  gdk_display_sync(gdk_display_get_default());
  g_object_unref (pixmap);
  g_object_unref (mask);
  g_free(foreground_color_p);
  g_free(background_color_p);
  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
}


/* Create pixmap and mask for the eraser cursor */
void get_eraser_pixmaps(int size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  *mask =  gdk_pixmap_new (NULL, size, size, 1);
  int circle_width = 2; 
  cairo_t *eraser_cr = gdk_cairo_create(*pixmap);
  cairo_set_source_rgb(eraser_cr, 1, 1, 1);
  cairo_paint(eraser_cr);
  cairo_stroke(eraser_cr);
  cairo_destroy(eraser_cr);

  cairo_t *eraser_shape_cr = gdk_cairo_create(*mask);
  clear_cairo_context(eraser_shape_cr);
  cairo_set_operator(eraser_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(eraser_shape_cr, circle_width);
  cairo_set_source_rgb(eraser_shape_cr, 1, 1, 1);
  cairo_arc(eraser_shape_cr, size/2, size/2, (size/2)-circle_width, 0, 2 * M_PI);
  cairo_stroke(eraser_shape_cr);
  cairo_destroy(eraser_shape_cr);
}


/* Set the eraser cursor */
void set_eraser_cursor()
{
  #ifdef _WIN32
    annotate_release_pointer_grab();
  #endif
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }
  gint size = data->cur_context->width;

  GdkPixmap *pixmap, *mask;
  get_eraser_pixmaps(size, &pixmap, &mask); 
  
  GdkColor *background_color_p = rgb_to_gdkcolor("000000");
  GdkColor *foreground_color_p = rgb_to_gdkcolor("FF0000");
 
  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, foreground_color_p, background_color_p, size/2, size/2);
  
  gdk_window_set_cursor (data->win->window, data->cursor);
  gdk_display_sync(gdk_display_get_default());
  g_object_unref (pixmap);
  g_object_unref (mask);
  g_free(foreground_color_p);
  g_free(background_color_p);
  #ifdef _WIN32
    annotate_acquire_pointer_grab();
  #endif
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


/* Unhide the cursor */
void unhide_cursor()
{
  if (data->cursor_hidden)
    {
      #ifdef _WIN32
        annotate_release_pointer_grab();
      #endif
      if(data->cur_context && data->cur_context->type == ANNOTATE_ERASER)
	    {
	      set_eraser_cursor();
	    } 
      else
	    {
	      set_pen_cursor();
	    }
      #ifdef _WIN32
        annotate_acquire_pointer_grab();
      #endif
      data->cursor_hidden = FALSE;  
    }
}


/* Grab the cursor */
void annotate_acquire_grab ()
{
  if  (!data->is_grabbed)
    {
	  gtk_window_present(GTK_WINDOW(get_bar_window()));
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

/* Set arrow */
void annotate_set_arrow(gboolean arrow)
{
  data->arrow = arrow;
}


/* Start to paint */
void annotate_toggle_grab()
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
  annotate_acquire_grab();
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
  
  add_save_point();
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
}


/* Draw an arrow using some polygons */
void annotate_draw_arrow ()
{
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
	
  int i = lenght-1;
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

/* Device lease */
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


/* This is called when the button is pushed */
G_MODULE_EXPORT gboolean
paint (GtkWidget *win,
       GdkEventButton *ev, 
       gpointer user_data)
{ 
  if (!ev)
    {
       return TRUE;
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
      return TRUE;
    }  

  reset_cairo();
  cairo_move_to(data->cr,ev->x,ev->y);
  if (inside_bar_window(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return TRUE;
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
       return TRUE;
    }
  unhide_cursor();

  if (inside_bar_window(ev->x, ev->y))
    /* point is in the ardesia bar */
    {
       if (data->debug)
         {    
           g_printerr("Device '%s': Move on the bar then ungrab\n",
	              ev->device->name);
         }
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return TRUE;
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
      return TRUE;
    }
  if (!(ev->device->source == GDK_SOURCE_MOUSE))
    {
      gint width = (CLAMP (pressure * pressure,0,1) *
		    (double) data->cur_context->width);
      if (width!=data->maxthckness)
        {
          data->maxthckness = width; 
          cairo_set_line_width(data->cr, data->maxthckness);
        }
    }
  annotate_draw_line (data->lastx, data->lasty, ev->x, ev->y, TRUE);
   
  annotate_coord_list_append (ev->x, ev->y, data->maxthckness);

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
	
  /* Delete the last line drawn*/
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


/* Call the geometric shape recognizer */
gboolean shape_recognize()
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
  return closed_path;
}

void draw_point(int x, int y)
{
  cairo_arc (data->cr, x, y, data->cur_context->width/2, 0, 2 * M_PI);
  cairo_fill (data->cr);
  cairo_move_to (data->cr, x, y);
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
       return TRUE;
    }
  /* only button1 allowed */
  if (!(ev->button==1))
    {
      return TRUE;
    }  

  if (inside_bar_window(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return TRUE;
    }

  if (!data->coordlist)
    {
      draw_point(ev->x, ev->y);
      data->lasty = ev->y;
      data->lastx = ev->x;	  
      annotate_coord_list_append (ev->x, ev->y, data->maxthckness);
    }
  else
    { 
      AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*)data->coordlist->data;
	  /* This is the tollerance to force to close the path in a magnetic way */
      int tollerance = 15;
 
      /* If the distance between two point lesser that tollerance they are the same point for me */
      if ((abs(ev->x-first_point->x)<tollerance) &&(abs(ev->y-first_point->y)<tollerance))
	  {
	    cairo_line_to(data->cr, first_point->x, first_point->y);
	  }
    }

  if (data->coordlist)
    {  
      gboolean closed_path = shape_recognize();
      if (!closed_path)
        {
          /* If is selected an arrowtype the draw the arrow */
	      if (data->arrow)
            {
	          /* print arrow at the end of the line */
	          annotate_draw_arrow ();
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
	  /* initialize a transparent window */
      data->cr = gdk_cairo_create(data->win->window);
      annotate_clear_screen();
					 
      data->transparent_pixmap = gdk_pixmap_new (data->win->window, data->width,
					   data->height, -1);
      data->transparent_cr = gdk_cairo_create(data->transparent_pixmap);
      clear_cairo_context(data->transparent_cr);
      restore_surface();
      /* This allows the mouse event to be passed to the window below at the start of the tool */
      gdk_window_input_shape_combine_mask (data->win->window, data->shape, 0, 0);
    }
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


/* Quit the annotation */
void annotate_quit()
{
  /* ungrab all */
  annotate_release_grab(); 
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
  
  /* disconnect the signals */
  g_signal_handlers_disconnect_by_func (data->win,
					G_CALLBACK (event_expose), NULL);
  
  /* free all */
  if (data->win)
    {
      gtk_widget_destroy(data->win); 
      data->win = NULL;
    }
  
  if (data->transparent_pixmap)
    {
      g_object_unref(data->transparent_pixmap);
      data->transparent_pixmap = NULL;
    }

  annotate_coord_list_free();
  annotate_savelist_free();

  g_free(data->default_pen->fg_color);
  g_free(data->default_pen);
  g_free(data->default_eraser);
  g_free (data);

  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor=NULL;
    }
}


/* Setup the application */
void setup_app ()
{ 
  data->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for(GTK_WINDOW(data->win), GTK_WINDOW(get_background_window()));

  gtk_widget_set_usize(GTK_WIDGET (data->win), data->width, data->height);
  gtk_window_fullscreen(GTK_WINDOW(data->win)); 
  
  if (STICK)
    {
      gtk_window_stick(GTK_WINDOW(data->win));
    }  
	
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(data->win), TRUE);

  #ifndef _WIN32 
    gtk_window_set_opacity(GTK_WINDOW(data->win), 1); 
  #else
    // TODO In windows I am not able yet to use an rgba transparent  
    // windows and then I use a sort of semi transparency 
    gtk_window_set_opacity(GTK_WINDOW(data->win), 0.5); 
  #endif  
 
  
  // initialize pen context
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN, 15);
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER, 15);
  data->cur_context = data->default_pen;
  data->state = 0;

  setup_input_devices();
  
  /* Initialize a transparent pixmap with depth 1 to be used as input shape */
  data->shape = gdk_pixmap_new (NULL, data->width, data->height, 1); 
  data->shape_cr = gdk_cairo_create(data->shape);
  clear_cairo_context(data->shape_cr); 

  /* connect the signals */
  annotate_connect_signals();

  gtk_widget_show_all(data->win);
 
  gtk_widget_set_app_paintable(data->win, TRUE);
  gtk_widget_set_double_buffered(data->win, FALSE);

}


/* Init the annotation */
int annotate_init (int x, int y, int width, int height, gboolean debug)
{
  data = g_malloc (sizeof(AnnotateData));
  data->cr = NULL;
  data->coordlist = NULL;
  data->savelist = NULL;
  data->transparent_pixmap = NULL;
  data->cursor = NULL;
  data->transparent_cr = NULL;
  
  data->debug = debug;
  
  /* Untoggle zone is setted on ardesia zone */
  data->untogglexpos = x;
  data->untoggleypos = y;
  data->untogglewidth = width;
  data->untoggleheight = height;
  
  data->cursor_hidden = FALSE;
  data->is_grabbed = FALSE;
  data->arrow = FALSE; 
  data->rectify = FALSE;
  data->roundify = FALSE;
  
  data->display = gdk_display_get_default();
  data->screen = gdk_display_get_default_screen(data->display);

  data->width = gdk_screen_get_width(data->screen);
  data->height = gdk_screen_get_height (data->screen);
  
  setup_app();

  return 0;
}
