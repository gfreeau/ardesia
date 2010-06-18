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


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>

#include <glib.h>
#include <gdk/gdkinput.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <cairo.h>


#ifdef _WIN32
  #include <cairo-win32.h>
  #include <gdkwin32.h>
  #include <winuser.h> 
  BOOL (WINAPI *setLayeredWindowAttributesProc) (HWND hwnd, COLORREF crKey,
	BYTE bAlpha, DWORD dwFlags) = NULL;
#else
  #ifdef __APPLE__
    #include <cairo-quartz.h>
  #else
    #include <cairo-xlib.h>
  #endif
#endif


#define ANNOTATE_MOUSE_EVENTS    ( GDK_PROXIMITY_IN_MASK   |    \
                                   GDK_PROXIMITY_OUT_MASK  |    \
                                   GDK_POINTER_MOTION_MASK |    \
                                   GDK_BUTTON_PRESS_MASK   |    \
                                   GDK_BUTTON_RELEASE_MASK      \
                                 )


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
  gchar*          fg_color;
} AnnotatePaintContext;


/* Struct to store the save point */
typedef struct _AnnotateSave
{
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
  GtkWidget *annotation_window;

  /* cairo context attached to the window */
  cairo_t *annotation_cairo_context;

  /* the shape pixmap used as mask */   
  GdkPixmap   *shape;

  /* cairo context attached to the shape pixmap */
  cairo_t     *shape_cr;

  /* transparent pixmap */
  GdkPixmap *transparent_pixmap;

  /* transparent cairo context */
  cairo_t *transparent_cr;
 
  /* mouse cursor */ 
  GdkCursor *cursor;
 
  /* mouse invisible cursor */ 
  GdkCursor *invisible_cursor;
 
  /* List of the savepoint */ 
  AnnotateSave *savelist;

  /* Paint context */ 
  AnnotatePaintContext *default_pen;
  AnnotatePaintContext *default_eraser;
  AnnotatePaintContext *cur_context;

  int thickness; 

  /* list of the coodinates of the last line drawn */
  GSList       *coordlist;

  /* input device */
  GdkDevice   *device;

  /* width of the screen */
  guint        width;
  guint        height;
 
  /* post draw operation */ 
  gboolean     rectify;
  gboolean     roundify;

  /* arrow */
  gboolean     arrow;
  
  /* is the cursor grabbed */
  gboolean     is_grabbed;
  
  /* is the cursor hidden */
  gboolean     cursor_hidden;
  
  /* is the debug enabled */
  gboolean     debug;

} AnnotateData;


/* Struct to store the painted point */
typedef struct
{
  gint x;
  gint y;
  gint width;
} AnnotateStrokeCoordinate;


/* Initialize the annotation window */
int annotate_init (GtkWidget* parent, gboolean debug);

/* Get the annotation window */
GtkWidget* get_annotation_window();

/* Hide the annotations */
void annotate_hide_annotation ();

/* Show the annotations */
void annotate_show_annotation ();

/* Get cairo context that contains the annotations */
cairo_t* get_annotation_cairo_context();

/* Get the cairo context that contains the background */
cairo_t* get_annotation_cairo_background_context();

/* Set the cairo context that contains the background */
void set_annotation_cairo_background_context(cairo_t* background_cr);

/* Undo to the last save point */
void annotate_undo();

/* Redo to the last save point */
void annotate_redo();

/* Quit the annotation */
void annotate_quit();

/* Set the pen color */
void annotate_set_color(gchar* color);

/* Set line width */
void annotate_set_width(guint width);

/* Set rectifier */
void annotate_set_rectifier(gboolean rectify);

/* Set rounder */
void annotate_set_rounder(gboolean rounder);

/* fill the last shape if it is a close path */
void annotate_fill();

/* Set arrow */
void annotate_set_arrow(gboolean arrow);

/* Set pen cursor */
void annotate_set_pen_cursor();

/* Set eraser cursor */
void annotate_set_eraser_cursor();

/* Start to paint */
void annotate_toggle_grab();

/* Start to erase */
void annotate_eraser_grab();

/* Release pointer grab */
void annotate_release_grab();

/* Acquire pointer grab */
void annotate_acquire_grab();

/* Clear the annotations windows */
void annotate_clear_screen();

/* Paint the context over the annotation window */
void merge_context(cairo_t * cr);


