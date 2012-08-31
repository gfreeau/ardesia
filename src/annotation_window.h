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

#ifndef ANNOTATE_H
#define ANNOTATE_H

#include <glib.h>

#include <gtk/gtk.h>

#include <cairo.h>

#ifdef _WIN32
#  include <cairo-win32.h>
#  include <gdkwin32.h>
#  include <winuser.h>
#else
#  ifdef __APPLE__
#    include <cairo-quartz.h>
#  else
#    include <cairo-xlib.h>
#  endif
#endif


#ifdef _WIN32
#  define ANNOTATION_UI_FOLDER "..\\share\\ardesia\\ui"
#  define ANNOTATION_UI_FILE ANNOTATION_UI_FOLDER"\\annotation_window.glade"

#else
#  define ANNOTATION_UI_FOLDER PACKAGE_DATA_DIR"/ardesia/ui"
#  define ANNOTATION_UI_FILE ANNOTATION_UI_FOLDER"/annotation_window.glade"
#endif


/* Enumeration containing tools. */
typedef enum
  {

    ANNOTATE_PEN,

    ANNOTATE_ERASER,

  } AnnotatePaintType;


/* Paint context. */
typedef struct
{

  AnnotatePaintType type;

  gchar *fg_color;

} AnnotatePaintContext;


/* Structure to store the save-point. */
typedef struct _AnnotateSavePoint
{

  /* The file name that represents the save-point. */
  gchar *filename;

} AnnotateSavepoint;

typedef struct
{

  /* List of the coordinates of the last line drawn. */
  GSList       *coord_list;

} AnnotateDeviceData;


/* Annotation data used by the callbacks. */
typedef struct
{

  /* Gtkbuilder for annotation window. */
  GtkBuilder *annotation_window_gtk_builder;

  /* Directory where store the save-point. */
  gchar* savepoint_dir;

  /* The annotation window. */
  GtkWidget *annotation_window;

  /* The cairo context attached to the window. */
  cairo_t *annotation_cairo_context;

  /* The back buffer surface used to do the input shape combine region. */
  cairo_surface_t *annotation_backsurface;

  /* Mouse cursor to be used. */
  GdkCursor *cursor;

  /* Mouse invisible cursor. */
  GdkCursor *invisible_cursor;

  /* List of the savepoint. */
  GSList  *savepoint_list;

  /*
   * The index of the position in the save-point list
   * of the current picture shown. 
   */
  guint    current_save_index;

  /* Hashtable that contains device dependant info. */
  GHashTable  *devdatatable;

  /* Paint context for the pen. */
  AnnotatePaintContext *default_pen;

  /* Paint context for the eraser. */
  AnnotatePaintContext *default_eraser;

  /* Point to the current context. */
  AnnotatePaintContext *cur_context;
  
  /*
   * This store the old paint type tool;
   * it is used to switch from/to eraser/pen
   * using a tablet pen.
   */
  AnnotatePaintType old_paint_type;
  
  /* Tool thickness. */
  gdouble thickness;

  /* Is the rectify mode enabled? */
  gboolean     rectify;
  
  /* Is the roundify mode enabled?*/
  gboolean     roundify;

  /* Arrow. */
  gboolean     arrow;

  /* Is the cursor grabbed. */
  gboolean     is_grabbed;

  /* Is the cursor hidden. */
  gboolean     is_cursor_hidden;

  /* Is the debug enabled. */
  gboolean     debug;


} AnnotateData;


/* Initialize the annotation cairo context */
void
initialize_annotation_cairo_context (AnnotateData *data);


/* Initialize the annotation window. */
gint
annotate_init                (GtkWidget *parent,
                              gchar     *iwb_filename,
                              gboolean   debug);


/* Set-up input device. */
void
setup_input_devices          ();


/* Get the annotation window. */
GtkWidget *
get_annotation_window        ();


/* Set the cairo context that contains the background. */
void
set_annotation_cairo_background_context (cairo_t *background_cr);


/* Draw the last save point on the window restoring the surface. */
void
annotate_restore_surface     ();


/* Get the cairo context that contains the background. */
cairo_t *
get_annotation_cairo_background_context ();


/* Paint the context over the annotation window. */
void
annotate_push_context        (cairo_t *cr);


/* Free the coord list belonging to the the owner devdata device. */
void
annotate_coord_dev_list_free (AnnotateDeviceData *devdata);


/* Free the coord list belonging to all the devices. */
void
annotate_coord_list_free     ();


/* Undo to the last save point. */
void
annotate_undo                ();


/* Redo to the last save point. */
void
annotate_redo                ();


/* Quit the annotation. */
void
annotate_quit                ();


/* Set the pen colour. */
void
annotate_set_color           (gchar              *color);


/* Modify colour according to the pressure. */
void
annotate_modify_color        (AnnotateDeviceData *devdata,
                              AnnotateData       *data,
                              gdouble             pressure);


/* Set the line thickness. */
void annotate_set_thickness  (gdouble thickness);


/* Get the line thickness. */
gdouble
annotate_get_thickness       ();


/* Set rectifier. */
void
annotate_set_rectifier       (gboolean rectify);


/* Set rounder. */
void
annotate_set_rounder         (gboolean rounder);


/* fill the last shape if it is a close path. */
void
annotate_fill                ();


/* Set arrow. */
void
annotate_set_arrow           (gboolean arrow);


/* Start to paint. */
void
annotate_toggle_grab         ();


/* Start to erase. */
void
annotate_eraser_grab         ();


/* Release pointer grab. */
void
annotate_release_grab        ();


/* Acquire pointer grab. */
void annotate_acquire_grab   ();


/* Clear the annotations windows. */
void
annotate_clear_screen        ();


/* Set a new cairo path with the new options. */
void
annotate_reset_cairo         ();


/* Hide the cursor. */
void
annotate_hide_cursor         ();


/* Un-hide the cursor. */
void
annotate_unhide_cursor       ();


/*
 * Add to the coordinate list the point (x,y)
 * storing the line width and the pressure.
 */
void
annotate_coord_list_prepend  (AnnotateDeviceData *devdata,
                              gdouble             x,
                              gdouble             y,
                              gdouble             width,
                              gdouble             pressure);


/* Draw line from the last point drawn to (x2,y2). */
void
annotate_draw_line           (AnnotateDeviceData *devdata,
                              gdouble             x2,
                              gdouble             y2,
                              gboolean            stroke);


/* Draw a point in x,y respecting the context. */
void
annotate_draw_point          (AnnotateDeviceData *devdata,
                              gdouble             x,
                              gdouble             y,
                              gdouble             pressure);


/* Draw an arrow using some polygons. */
void
annotate_draw_arrow          (AnnotateDeviceData *devdata,
                              gdouble             distance);


/* Select eraser, pen or other tool for tablet. */
void
annotate_select_tool         (AnnotateData      *data,
                              GdkDevice         *device,
                              guint              state);


/* Select the default pen tool. */
void
annotate_select_pen          ();


/* Select the default eraser tool. */
void
annotate_select_eraser       ();


/* Call the geometric shape recognizer. */
void
annotate_shape_recognize     (AnnotateDeviceData *devdata,
                              gboolean            closed_path);


/* Add a save point for the undo/redo. */
void
annotate_add_savepoint       ();


/* Configure pen option for cairo context. */
void 
annotate_configure_pen_options    (AnnotateData       *data);


#endif


