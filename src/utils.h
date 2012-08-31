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


#include <glib.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include <errno.h>

#include <string.h>

#include <math.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>


#ifdef _WIN32
#  include <cairo-win32.h>
#else
#  ifdef __APPLE__
#    include <cairo-quartz.h>
#  else
#    include <cairo-xlib.h>
#  endif
#endif


#include <config.h>


/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#else
#  define gettext ( char* ) ( char* )
#endif


/* The gtk builder object of the bar window */
GtkBuilder *bar_gtk_builder;

		
#define PROGRAM_NAME "Ardesia"

/* Color definition in RGB. */
#define BLACK "000000"
#define WHITE "FFFFFF"
#define RED   "FF0000"
#define YELLOW "FFFF00"
#define GREEN "00FF00"
#define BLUE "0000FF"



/* Struct to store the painted point. */
typedef struct
{
  gdouble x;
  gdouble y;
  gdouble width;
  gdouble pressure;
} AnnotatePoint;


/* Get the name of the current project. */
gchar *
get_project_name        ();


/* Set the name of the current project. */
void
set_project_name        (gchar *name);


/* Get the directory of the current project. */
gchar *
get_project_dir         ();


/* Set the directory of the current project. */
void
set_project_dir         (gchar *dir);


/* Get the iwb file name of the current project. */
gchar *
get_iwb_filename        ();


/* Set the iwb file name of the current project. */
void
set_iwb_filename        (gchar *file);


/* Get the list of the path of the artefacts created in the session. */
GSList *
get_artifacts           ();


/* Add the path of an artefacts created in the session to the list. */
void add_artifact       (gchar* path);


/* Free the structure containing the artefact list created in the session. */
void
free_artifacts          ();

				
/* get bar window widget. */
GtkWidget *
get_bar_window          ();


/* Take a GdkColor and return the RGB string. */
gchar *
gdkcolor_to_rgb         (GdkColor *gdkcolor);


/* Scale the surface with the width and height requested */
cairo_surface_t *
scale_surface           (cairo_surface_t  *surface,
                         gdouble           width,
                         gdouble           height);
				
				
/* Set the cairo surface color to the RGBA string. */
void
cairo_set_source_color_from_string     (cairo_t  *cr,
                                        gchar    *color);


/* Set the cairo surface color to transparent. */
void
cairo_set_transparent_color       (cairo_t  *cr);


/* Distance between two points using the Pitagora theorem. */
gdouble
get_distance            (gdouble  x1,
                         gdouble  y1,
                         gdouble  x2,
                         gdouble  y2);


/* Clear cairo context. */
void
clear_cairo_context     (cairo_t  *cr);


/*
 * This is function return if the point (x,y) in inside the ardesia bar window.
 */
gboolean
inside_bar_window       (gdouble  xp,
                         gdouble  yp);


/* Drill the gdkwindow in the area where the ardesia bar is located. */
void
drill_window_in_bar_area     (GtkWidget  *widget);


/*
 * Take a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel.
 */
GdkColor *
rgba_to_gdkcolor        (gchar  *rgb);


/* Save the contents of the pixbuf in the file with name file name. */
gboolean
save_pixbuf_on_png_file      (GdkPixbuf    *pixbuf,
                              const gchar  *filename);


/* Grab the screenshoot and put it in the image buffer. */
GdkPixbuf *
grab_screenshot         ();


/* 
 * Return a file name containing
 * the project name and the current date.
 *
 */
gchar *
get_default_filename    ();


/*
 * Get the current date and format in a printable format;
 * the returned value must be free with the g_free.
 */
gchar *
get_date           ();


/* Return if a file exists. */
gboolean
file_exists        (gchar*  filename);


/*
 * Get the home directory.
 */
const gchar *
get_home_dir       (void);


/*
 * Get the desktop directory.
 */
const gchar *
get_desktop_dir     (void);


/*
 * Get the documents directory.
 */
const gchar *
get_documents_dir       (void);


/* Remove recursive a directory. */
void
rmdir_recursive         (gchar  *path);


/* Remove directory if it is empty */
void
remove_dir_if_empty     (gchar  *dir_path);


/* Allocate a new point belonging to the path passing the values. */
AnnotatePoint *
allocate_point     (gdouble  x,
                    gdouble  y,
                    gdouble  width,
                    gdouble  pressure);


/* Send an email. */
void
send_email         (gchar   *to,
                    gchar   *subject,
                    gchar   *body,
                    GSList  *attachment_list);


/* Send artefacts with email. */
void
send_artifacts_with_email    (GSList  *attachment_list);


/* Send trace with email. */
void
send_trace_with_email        (gchar  *attachment);


/* Is the desktop manager gnome. */
gboolean
is_gnome ();


/* Create desktop entry passing value. */
void
xdg_create_desktop_entry     (gchar  *filename,
                              gchar  *type,
                              gchar  *name,
                              gchar  *icon,
                              gchar  *exec);


/* Create a desktop link. */
void
xdg_create_link         (gchar  *src_filename,
                         gchar  *dest,
                         gchar  *icon);


/* Get the last position where sub-string occurs in string. */
int
g_substrlastpos         (const char  *str,
                         const char  *substr);


/* Sub-string of string from start to end position. */
gchar *
g_substr           (const gchar  *string,
                    gint          start,
                    gint          end);


/* 
 * This function create a segmentation fault; 
 * it is useful to test the segmentation fault handler.
 */
void
create_segmentation_fault    ();


