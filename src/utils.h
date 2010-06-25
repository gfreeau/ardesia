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



#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>


#ifdef _WIN32
  #include <cairo-win32.h>
  #include <windows_utils.h>
#else
  #ifdef __APPLE__
    #include <cairo-quartz.h>
  #else
    #include <cairo-xlib.h>
  #endif
#endif

#include <config.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


#ifdef _WIN32
  #define DIR_SEPARATOR '\\'
#else
  #define DIR_SEPARATOR '/'
#endif


extern GtkBuilder *gtkBuilder;

		
#define PROGRAM_NAME "Ardesia"

#define BLACK "000000"
#define WHITE "FFFFFF"
#define RED   "FF0000"
#define YELLOW "FFFF00"
#define GREEN "00FF00"
#define BLUE "0000FF"

				
/* get bar window widget */
GtkWidget* get_bar_window();


/* Take a GdkColor and return the RGB string */
gchar* gdkcolor_to_rgba(GdkColor* gdkcolor);


/* Set the cairo surface color to the RGBA string */
void cairo_set_source_color_from_string( cairo_t * cr, char* color);


/* Set the cairo surface color to transparent */
void  cairo_set_transparent_color(cairo_t * cr);


/** Distance beetween two points using the Pitagora theorem */
int get_distance(int x1, int y1, int x2, int y2);


/* Clear cairo context */
void clear_cairo_context(cairo_t* cr);


/*
 * This is function return if the point (x,y) in inside the ardesia bar window
 */
gboolean inside_bar_window(int xp, int yp);


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel
 */
GdkColor* rgb_to_gdkcolor(char* rgb);


/* Get the current date and format in a printable format */
gchar* get_date();


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir);


/*
 * Get the desktop folder;
 * this function use gconf to found the folder
 */
const gchar* get_desktop_dir (void);


