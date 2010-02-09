/* 
 * Ardesia-- a program for painting on the screen 
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * Some parts of this file are been copied from gromit
 * Copyright (C) 2000 Simon Budig <Simon.Budig@unix-ag.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk/gdkinput.h>
#include <gdk/gdkx.h>
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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <annotate.h>
#include <broken.h>
#include <bezier_spline.h>


#define ANNOTATE_MOUSE_EVENTS    ( GDK_PROXIMITY_IN_MASK |      \
				   GDK_PROXIMITY_OUT_MASK |	\
				   GDK_POINTER_MOTION_MASK |	\
				   GDK_BUTTON_PRESS_MASK |      \
				   GDK_BUTTON_RELEASE_MASK      \
				   )

#define ANNOTATE_KEYBOARD_EVENTS ( GDK_KEY_PRESS_MASK	\
				   )



/* set the DEBUG to 1 to read the debug messages */
#define DEBUG 0

#define g_slist_first(n) g_slist_nth (n,0)

GdkPixmap *transparent_pixmap = NULL;

typedef enum
  {
    ANNOTATE_PEN,
    ANNOTATE_ERASER,
  } AnnotatePaintType;


typedef struct
{
  AnnotatePaintType type;
  guint           width;
  guint           arrowsize;
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
} AnnotateData;

AnnotateData* data;


/* Create a new paint context */
AnnotatePaintContext * annotate_paint_context_new (AnnotatePaintType type,
			                           char* fg_color, 
                                                   guint width, 
                                                   guint arrowsize)
{
  AnnotatePaintContext *context;
  context = malloc (sizeof (AnnotatePaintContext));
  context->type = type;
  context->width = width;
  context->arrowsize = arrowsize;
  context->fg_color = fg_color;
  return context;
}


/* Get the annotation window */
GtkWindow* get_annotation_window()
{
  return GTK_WINDOW(data->win);
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
  g_printerr ("arrowsize: %3d, ", context->arrowsize);
  g_printerr ("color: %s\n", context->fg_color);
}


/* Free the memory allocated by paint context */
void annotate_paint_context_free (AnnotatePaintContext *context)
{
  free (context);
}


/* Add to the list of the painted point the point (x,y) storing the width */
void annotate_coord_list_prepend (gint x, gint y, gint width)
{
  AnnotateStrokeCoordinate *point;
  point = malloc (sizeof (AnnotateStrokeCoordinate));
  point->x = x;
  point->y = y;
  point->width = width;
  data->coordlist = g_slist_prepend (data->coordlist, point);
}




/* Free the list of the painted point */
void annotate_coord_list_free ()
{
  GSList *ptr = data->coordlist;
  g_slist_foreach(ptr, (GFunc)free, NULL);
  g_slist_free(ptr);
  data->coordlist = NULL;
}


/* Free the list of the savepoint  */
void annotate_savelist_free ()
{
  AnnotateSave* annotate_save = data->savelist;

  while (annotate_save)
    {
      annotate_save =  annotate_save->previous; 
    }
  /* annotate_save point to the first save point */

  while(annotate_save)
    {
      cairo_surface_destroy(annotate_save->surface); 
      free(annotate_save);
      annotate_save =  annotate_save->next; 
    }
  /* all the savepoint might be deleted */

  data->savelist = NULL;
}


/* Free the list of the  savepoint for the redo */
void annotate_redolist_free ()
{
  AnnotateSave* annotate_save = data->savelist->next;
  /* delete all the savepoint after the current pointed */
  while(annotate_save)
    { 
      cairo_surface_destroy(annotate_save->surface); 
      free(annotate_save);
      annotate_save =  annotate_save->next; 
    }  
}


/* Calculate parameters for the arrow */
gboolean annotate_coord_list_get_arrow_param (AnnotateData* data,
				     	      gint        search_radius,
				              gint       *ret_width,
				              gfloat     *ret_direction)
{
  gint x0, y0, r2, dist;
  gboolean success = FALSE;
  AnnotateStrokeCoordinate  *cur_point, *valid_point;
  GSList *ptr = data->coordlist;
  guint width;

  valid_point = NULL;

  if (ptr)
    {
      cur_point = ptr->data;
      x0 = cur_point->x;
      y0 = cur_point->y;
      r2 = pow(search_radius, 2);
      dist = 0;

      while (ptr && dist < r2)
        {
          ptr = ptr->next;
          if (ptr)
            {
              cur_point = ptr->data;
              dist = pow(cur_point->x - x0, 2) + pow(cur_point->y - y0, 2);
              width = cur_point->width * data->cur_context->arrowsize;
              if(!valid_point || valid_point->width < cur_point->width)
		{
		  valid_point = cur_point;
                }
            }
        }

      if (valid_point)
        {
          *ret_width = MAX (valid_point->width * data->cur_context->arrowsize, 2);
          *ret_direction = atan2 (y0 - valid_point->y, x0 - valid_point->x);
          success = TRUE;
        }
    }

  return success;
}

/* Set the cairo surface color to the RGBA string */
void cairo_set_source_color_from_string( cairo_t * cr, char* color)
{
  int r,g,b,a;
  sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);
  cairo_set_source_rgba (cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);
}


/* Set the cairo surface color to transparent */
void  cairo_set_transparent_color(cairo_t * cr)
{
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
}


/* Clear cairo context */
void clear_cairo_context(cairo_t* cr)
{
  cairo_save(cr);
  cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
  cairo_set_transparent_color(cr);
  cairo_paint(cr);
  cairo_restore(cr);
}


/* Repaint the window */
gint add_save_point ()
{
  AnnotateSave* annotate_save = data->savelist;
  cairo_surface_t* saved_surface = annotate_save->surface;
  cairo_surface_t* source_surface = cairo_get_target(data->cr);
  cairo_t *cr = cairo_create (saved_surface);
  cairo_set_source_surface (cr, source_surface, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
  return 1;
}


void restore_surface()
{
  AnnotateSave* annotate_save = data->savelist;
  cairo_surface_t* saved_surface = annotate_save->surface;
  cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (data->cr, saved_surface, 0, 0);
  cairo_paint(data->cr);
}


/* Undo to the last save point */
void annotate_undo()
{
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
  if (data->savelist)
    {
      if (data->savelist->next)
        {
          data->savelist = data->savelist->next;
          restore_surface(); 
        }
    }
}


/* Undo and put the cursor in the old position */
void annotate_undo_and_restore_pointer()
{
  annotate_undo();
  GdkScreen   *screen = gdk_display_get_default_screen (data->display);
  gdk_display_warp_pointer (data->display, screen, data->savelist->xcursor, data->savelist->ycursor); 
}


/* Hide the annotations */
void annotate_hide_annotation ()
{

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
  restore_surface();
}


/* Releare keyboard grab */
void annotate_release_keyboard_grab()
{
  gdk_display_keyboard_ungrab (data->display, GDK_CURRENT_TIME);
  gdk_flush ();
}


/* Acquire keyboard grab */
void annotate_acquire_keyboard_grab()
{
  gdk_keyboard_grab (data->win->window,
		     ANNOTATE_KEYBOARD_EVENTS,
		     GDK_CURRENT_TIME); 
  
  gdk_flush ();
}


/* Release the mouse pointer */
void annotate_release_grab ()
{           
  gdk_error_trap_push ();
 
  gdk_display_pointer_ungrab (data->display, GDK_CURRENT_TIME);
  annotate_release_keyboard_grab();
  /* inherit cursor from root window */
  gdk_window_set_cursor (data->win->window, NULL);
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      /* this probably means the device table is outdated, 
	 e.g. this device doesn't exist anymore */
      g_printerr("Error ungrabbing mouse device while ungrabbing all, ignoring.\n");
    }
  
  gtk_widget_input_shape_combine_mask(data->win, data->shape, 0, 0); 
  
  if(data->debug)
    {
      g_printerr ("DEBUG: Ungrabbed pointer.\n");
    }
}


/* Select the default eraser tool */
void annotate_select_eraser()
{
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
  gint size = 12;
  
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
   
  GdkColor background_color =  {0, 0xFFFF, 0xFFFF, 0xFFFF}; 
  GdkColor *foreground_color_p = rgb_to_gdkcolor(data->cur_context->fg_color);

  GdkCursor* cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap, foreground_color_p, &background_color, size/2 + context_width/2, 5* size/2);
  gdk_window_set_cursor (data->win->window, cursor);
  gdk_flush ();
  g_object_unref (pixmap);
  gdk_cursor_destroy (cursor);
  g_free(foreground_color_p);
  cairo_destroy(pen_cr);
}


/* Set the eraser cursor */
void set_eraser_cursor()
{
  gint size = data->cur_context->width;
  GdkPixmap *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  int circle_width = 2; 
  cairo_t *eraser_cr = gdk_cairo_create(pixmap);
  clear_cairo_context(eraser_cr);
  cairo_set_operator(eraser_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(eraser_cr, circle_width);
  cairo_set_source_rgba(eraser_cr,0,0,0,1);
  
  cairo_arc(eraser_cr, size/2, size/2, (size/2)-circle_width, 0, 2 * M_PI);
  cairo_stroke(eraser_cr);
  
  GdkColor background_color =  {0, 0xFFFF, 0xFFFF, 0xFFFF}; 
  GdkColor foreground_color =  {0, 0xFFFF, 0x0000, 0x0000 }; 
  GdkCursor* cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap, &foreground_color, &background_color, size/2, size/2);
  gdk_window_set_cursor (data->win->window, cursor);
  gdk_flush ();
  g_object_unref (pixmap);
  gdk_cursor_destroy (cursor);
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


/* Color selector; if eraser than select the transparent color else alloc the right color */ 
void select_color()
{
  if (data->cur_context)
    {
      if (!(data->cur_context->type == ANNOTATE_ERASER))
        {
          cairo_set_source_color_from_string(data->cr, data->cur_context->fg_color);
        }
      else
        {
          cairo_set_transparent_color(data->cr);
        }
   }
}


/* Grab the cursor */
void annotate_acquire_grab ()
{
  gtk_widget_show (data->win);

  GdkGrabStatus result;
  gdk_error_trap_push(); 

  gtk_widget_input_shape_combine_mask(data->win, NULL, 0, 0);   

  result = gdk_pointer_grab (data->win->window, FALSE,
			     ANNOTATE_MOUSE_EVENTS, 0,
			     NULL,
			     GDK_CURRENT_TIME); 

  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_printerr("Error grabbing pointer, ignoring.");
    }
   
  switch (result)
    {
    case GDK_GRAB_SUCCESS:
      break;
    case GDK_GRAB_ALREADY_GRABBED:
      g_printerr ("Grabbing Pointer failed: %s\n", "AlreadyGrabbed");
      break;
    case GDK_GRAB_INVALID_TIME:
      g_printerr ("Grabbing Pointer failed: %s\n", "GrabInvalidTime");
      break;
    case GDK_GRAB_NOT_VIEWABLE:
      g_printerr ("Grabbing Pointer failed: %s\n", "GrabNotViewable");
      break;
    case GDK_GRAB_FROZEN:
      g_printerr ("Grabbing Pointer failed: %s\n", "GrabFrozen");
      break;
    default:
      g_printerr ("Grabbing Pointer failed: %s\n", "Unknown error");
    }      
  if(data->cur_context && data->cur_context->type == ANNOTATE_ERASER)
    {
      set_eraser_cursor(data);
    } 
  else
    {
      set_pen_cursor();
    } 
  
  annotate_acquire_keyboard_grab();
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
  if (arrow>0)
    { 
      /* set the arrowsize with a function depending on line width */
      data->cur_context->arrowsize =  log(data->cur_context->width+7);
    }
  else
    {
      data->cur_context->arrowsize =  0;
    }
}


/* Start to paint */
void annotate_toggle_grab ()
{ 
  annotate_select_pen(data);
  annotate_acquire_grab (data);
}


/* Start to erase */
void annotate_eraser_grab ()
{
  /* Get the with message */
  annotate_select_eraser(data);
  annotate_configure_eraser(data->cur_context->width);
  annotate_acquire_grab (data);
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
  cairo_stroke(data->cr);
  AnnotateSave *save = malloc(sizeof(AnnotateSave));
  save->previous  = NULL;
  save->next  = NULL;
  save->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height); 

  if (data->savelist != NULL)
    {
      data->savelist->next=save;
      save->previous=data->savelist;
    }
  data->savelist=save;
  configure_pen_options();  
}


/* Clear the screen */
void clear_screen()
{
  reset_cairo();
  clear_cairo_context(data->cr);
  add_save_point();
}


/* Clear the annotations windows */
void annotate_clear_screen ()
{
  clear_screen();
  data->painted = FALSE;
}


/* Select eraser, pen or other tool for tablet */
void annotate_select_tool (GdkDevice *device, guint state)
{
  guint buttons = 0, modifier = 0, len = 0;
  guint req_buttons = 0, req_modifier = 0;
  guint i, j, success = 0;
  AnnotatePaintContext *context = NULL;
  guchar *name;
 
  if (device)
    {
      len = strlen (device->name);
      name = (guchar*) g_strndup (device->name, len + 3);
      
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
      g_printerr ("ERROR: Attempt to select nonexistent device!\n");
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
  data->painted = TRUE;
}


/* Draw an arrow using some polygons */
void annotate_draw_arrow (gint x1, gint y1,
		          gint width, gfloat direction)
{
  GdkPoint arrowhead [4];

  int penwidth = data->cur_context->width/2;
  int penwidthcos = 2 * penwidth * cos (direction);
  int penwidthsin = 2 * penwidth * sin (direction);
  int widthcos = penwidthcos;
  int widthsin = penwidthsin;

  /* Vertex of the arrow */
  arrowhead [0].x = x1 + penwidthcos;
  arrowhead [0].y = y1 + penwidthsin;

  /* left point */
  arrowhead [1].x = x1 - 2 * widthcos + widthsin ;
  arrowhead [1].y = y1 -  widthcos -  2 * widthsin ;

  /* origin */
  arrowhead [2].x = x1 - widthcos ;
  arrowhead [2].y = y1 - widthsin ;

  /* right point */
  arrowhead [3].x = x1 - 2 * widthcos - widthsin ;
  arrowhead [3].y = y1 +  widthcos - 2 * widthsin ;

  cairo_stroke(data->cr);
  cairo_save(data->cr);
  cairo_set_line_cap (data->cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(data->cr, CAIRO_LINE_JOIN_MITER); 
  cairo_set_operator(data->cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(data->cr, penwidth);
  select_color();  

  cairo_move_to(data->cr, arrowhead[2].x, arrowhead[2].y); 
  cairo_line_to(data->cr, arrowhead[1].x, arrowhead[1].y);
  cairo_line_to(data->cr, arrowhead[0].x, arrowhead[0].y);
  cairo_line_to(data->cr, arrowhead[3].x, arrowhead[3].y);

  cairo_close_path(data->cr);
  cairo_fill_preserve(data->cr);
  cairo_stroke(data->cr);
  cairo_restore(data->cr);
 
  data->painted = TRUE;
}


/* Draw a back arrow using some polygons */
void annotate_draw_back_arrow (gint x1, gint y1,
			       gint width, gfloat direction)
{
  AnnotateData *revertcoordata = malloc (sizeof (AnnotateData));
  revertcoordata->cur_context = data->cur_context;
  GSList *ptr = data->coordlist;
  if (ptr)
    {
      GSList *revptr = g_slist_reverse(ptr);
      revertcoordata->coordlist = revptr;
      AnnotateStrokeCoordinate  *first_point;
      first_point = revptr->data;
      int x0 = first_point->x;
      int y0 = first_point->y;
      annotate_coord_list_get_arrow_param (revertcoordata, width * 3,
					   &width, &direction);
      annotate_draw_arrow (x0, y0, width, direction);
    }
  free(revertcoordata);
}


/*
 * Event-Handlers to perform the drawing
 */

/* Device touch */
gboolean proximity_in (GtkWidget *win,
                       GdkEventProximity *ev, 
                       gpointer user_data)
{
  gint x, y;
  GdkModifierType state;

  gdk_window_get_pointer (data->win->window, &x, &y, &state);
  annotate_select_tool (ev->device, state);

  if(data->debug)
    {
      g_printerr("DEBUG: proximity in device  %s: \n", ev->device->name);
    }
  return TRUE;
}


/* Device lease */
gboolean proximity_out (GtkWidget *win, 
                        GdkEventProximity *ev,
                        gpointer user_data)
{
  
  data->cur_context = data->default_pen;

  data->state = 0;
  data->device = NULL;

  if(data->debug)
    {
      g_printerr("DEBUG: proximity out device  %s: \n", ev->device->name);
    }
  return FALSE;
}


/* This is called when the button is pushed */
gboolean paint (GtkWidget *win,
                GdkEventButton *ev, 
                gpointer user_data)
{ 
  annotate_coord_list_free (data);
  /* only button1 allowed */
  if (!(ev->button==1))
  {
      return FALSE;
  }  

  reset_cairo();
  
  if (in_unlock_area(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
      /* the last point was outside the bar then ungrab */
      annotate_release_grab ();
      return FALSE;
    }
 
  if(data->debug)
    {
      g_printerr("DEBUG: Device '%s': Button %i Down at (x,y)=(%.2f : %.2f)\n",
                 ev->device->name, ev->button, ev->x, ev->y);
    }
  cairo_arc (data->cr, ev->x, ev->y, data->cur_context->width/2, 0, 2 * M_PI);
  cairo_fill (data->cr);
  cairo_move_to (data->cr, ev->x, ev->y);
  data->lastx = ev->x;
  data->lasty = ev->y;
  annotate_coord_list_prepend (ev->x, ev->y, data->maxwidth);
  data->painted = TRUE;
  return TRUE;
}


/* This shots when the ponter is moving */
gboolean paintto (GtkWidget *win, 
                  GdkEventMotion *ev, 
                  gpointer user_data)
{

  if (in_unlock_area(ev->x,ev->y))
    /* point is in the ardesia bar */
    {
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
   
  annotate_coord_list_prepend (ev->x, ev->y, data->maxwidth);

  data->lastx = ev->x;
  data->lasty = ev->y;
 
  return TRUE;
}


/* This draw an ellipse taking the top left edge coordinates the width and the eight of the bounded rectangle */
void cairo_draw_ellipse(gint x, gint y, gint width, gint height)
{
  cairo_save(data->cr);
  cairo_translate (data->cr, x + width / 2., y + height / 2.);
  cairo_scale (data->cr, width / 2., height / 2.);
  cairo_arc (data->cr, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore(data->cr);
}


/** Draw the point list */
void draw_point_list(GSList* outptr)
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
      outptr = outptr ->next;   
    }
}


/* Rectify the line or the shape*/
gboolean rectify()
{
  annotate_undo();
  reset_cairo();
  gboolean close_path = FALSE;
  GSList *outptr = broken(data->coordlist, &close_path, TRUE);   
  annotate_coord_list_free (data);
  data->coordlist = outptr;
  draw_point_list(outptr);     
  if (close_path)
    {
      cairo_close_path(data->cr);   
    }
  return close_path;
}


/* Roundify the line or the shape*/
gboolean roundify()
{
  annotate_undo();
  reset_cairo();
  gboolean close_path = FALSE;
  GSList *outptr = broken(data->coordlist, &close_path, FALSE);   
  annotate_coord_list_free (data);
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
  if (data->cr)
    {
      if (!(data->roundify)&&(!(data->rectify)))
      {
        draw_point_list(data->coordlist);     
        cairo_close_path(data->cr);      
      }
      select_color();
      cairo_fill(data->cr);
      cairo_stroke(data->cr);
      add_save_point();
    }
}


/* This shots when the button is realeased */
gboolean paintend (GtkWidget *win, GdkEventButton *ev, gpointer user_data)
{
   
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

  
  gint width = data->cur_context->arrowsize * data->cur_context->width / 2;
  gfloat direction = 0;
 
  AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*)data->coordlist->data;
  int tollerance = 15;

  if ((abs(ev->x-first_point->x)<tollerance) &&(abs(ev->y-first_point->y)<tollerance))
    {
      cairo_line_to(data->cr, first_point->x, first_point->y);
    }
  else
    {
      cairo_line_to(data->cr, ev->x, ev->y);
    }

  cairo_stroke_preserve(data->cr);
  add_save_point();
  
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
          gboolean arrowparam =  annotate_coord_list_get_arrow_param (data, width * 3,
					       &width, &direction);
          /* If is selected an arrowtype the draw the arrow */
          if (data->arrow>0 && data->cur_context->arrowsize > 0 && arrowparam)
	    {
	      /* print arrow at the end of the line */
	      annotate_draw_arrow (ev->x, ev->y, width, direction);
	      if (data->arrow==2)
	        {
	          /* print arrow at the beginning of the line */
	          annotate_draw_back_arrow (ev->x, ev->y, width, direction);
	        }
	    }
        }

      cairo_stroke_preserve(data->cr);
      add_save_point();
    } 
 
  return TRUE;
}


/*
 * Functions for handling various (GTK+)-Events
 */

/* press keyboard event */
gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)  
{
 
  if (event->keyval == GDK_BackSpace)
    {
      // undo
      annotate_undo_and_restore_pointer();
      return FALSE;
    }
  
  reset_cairo();
  
  /* This is a trick we must found the maximum height and width of the font */
  cairo_text_extents_t extents;
  cairo_set_font_size (data->cr, data->cur_context->width * 5);
  cairo_text_extents (data->cr, "j" , &extents); 
   
  /* move cairo to the mouse position */
  int x;
  int y;
  gdk_display_get_pointer (data->display, NULL, &x, &y, NULL);  
 
  /* store the pointer position */ 
  data->savelist->previous->xcursor=x;  
  data->savelist->previous->ycursor=y;   
  
  cairo_move_to(data->cr, x, y);
  GdkScreen   *screen = gdk_display_get_default_screen (data->display);

  /* is finished the line or is pressed enter */
  if ((x + extents.x_advance >= data->width) ||
      (event->keyval == GDK_ISO_Enter) || 	
      (event->keyval == GDK_KP_Enter))
    {
      x = 0;
      y +=  extents.height;
      /* go to the new line */
      gdk_display_warp_pointer (data->display, screen, x, y);  
    } 
  else if (event->keyval == GDK_Left)
    {
      x -=  extents.x_advance;
      gdk_display_warp_pointer (data->display, screen, x, y); 
    }
  else if ((event->keyval == GDK_Right) ||  (event->keyval == GDK_KP_Space))
    {
      x +=  extents.x_advance;
      gdk_display_warp_pointer (data->display, screen, x, y); 
    }
  else if (event->keyval == GDK_Up)
    {
      y -=  extents.height;
      gdk_display_warp_pointer (data->display, screen, x, y); 
    }
  else if (event->keyval == GDK_Down)
    {
      y +=  extents.height;
      gdk_display_warp_pointer (data->display, screen, x, y); 
    }
  else if (isprint(event->keyval))
    {
      /* The character is printable */
      char *utf8 = malloc(2) ;
      sprintf(utf8,"%c", event->keyval);
      cairo_text_extents (data->cr, utf8, &extents);
      cairo_show_text (data->cr, utf8); 
      cairo_stroke(data->cr);
      add_save_point();
 
      /* move cursor to the x step */
      x +=  extents.x_advance;
      gdk_display_warp_pointer (data->display, screen, x, y);  
      data->savelist->xcursor=x;  
      data->savelist->ycursor=y;   
      annotate_redolist_free ();
      free(utf8);
    }
  return FALSE;
}


/* Expose event: this occurs when the windows is show */
gboolean event_expose (GtkWidget *widget, 
                       GdkEventExpose *event, 
                       gpointer user_data)
{

  data->cr = gdk_cairo_create(data->win->window);
  clear_screen();
   
  transparent_pixmap = gdk_pixmap_new (data->win->window, data->width,
						  data->height, -1);
  cairo_t *transparent_cr = gdk_cairo_create(transparent_pixmap);
  clear_cairo_context(transparent_cr);
  cairo_destroy(transparent_cr);
  restore_surface();
  return TRUE;
}


/* Do the gtk main loop */
void annotate_main_do_event (GdkEventAny *event, 
                             AnnotateData  *data)
{
  gtk_main_do_event ((GdkEvent *) event);
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


/* Setup the application */
void setup_app ()
{ 
  /* default color is opaque red */ 
  char *color ="FF0000FF";

  data->cr = NULL;
  data->display = gdk_display_get_default ();
  GdkScreen   *screen = gdk_display_get_default_screen (data->display);

  data->width = gdk_screen_get_width (screen);
  data->height = gdk_screen_get_height (screen);

  data->win = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_usize (GTK_WIDGET (data->win), data->width, data->height);
  gtk_widget_set_usize (GTK_WIDGET (data->win), data->width, data->height);
 
  gtk_window_set_opacity(GTK_WINDOW(data->win), 1); 
  gtk_widget_set_default_colormap(gdk_screen_get_rgba_colormap(screen));
 
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

  g_signal_connect (data->win, "key_press_event",
		    G_CALLBACK (key_press), NULL);

  data->arrow = 0; 
  data->painted = FALSE;
  data->rectify = FALSE;
  data->roundify = FALSE;

  data->coordlist = NULL;
  data->savelist = NULL;
  data->cr = NULL;
  
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN,
						  color, 15, 0);
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER,
						     NULL, 15, 0);

  data->cur_context = data->default_pen;

  data->state = 0;

  setup_input_devices (data);
  
  gtk_widget_show_all(data->win);
  
  gtk_widget_set_app_paintable(data->win, TRUE);
  gtk_widget_set_double_buffered(data->win, FALSE);
    
  /* SHAPE PIXMAP */
  data->shape = gdk_pixmap_new (NULL, data->width, data->height, 1); 
  cairo_t* shape_cr = gdk_cairo_create(data->shape);
  clear_cairo_context(shape_cr); 
  cairo_destroy(shape_cr);
  
  /* this allow the mouse focus below the transparent window */ 
  gtk_widget_input_shape_combine_mask(data->win, data->shape, 0, 0); 
}


/* Quit the annotation */
void annotate_quit()
{
  /* ungrab all */
  annotate_release_grab(); 
  /* destroy cairo */
  g_object_unref(data->shape);
  destroy_cairo();
  gtk_widget_destroy(data->win); 
  /* free all */
  g_object_unref(transparent_pixmap);
  annotate_coord_list_free ();
  annotate_savelist_free ();
  free(data->default_pen);
  free(data->default_eraser);
  free (data);
}


/* Init the annotation */
int annotate_init (int x, int y, int width, int height)
{
  data = malloc (sizeof (AnnotateData));
  data->debug = DEBUG;
  /* Untoggle zone is setted on ardesia zone */
  data->untogglexpos = x;
  data->untoggleypos = y;
  data->untogglewidth = width;
  data->untoggleheight = height;
 
  setup_app ();

  return 0;
}
