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


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <getopt.h>
#include <glib.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>

#include <gconf/gconf-client.h>


#ifdef _WIN32
  #include <cairo-win32.h>
  #include <winuser.h>  
#else
  #ifdef __APPLE__
    #include <cairo-quartz.h>
  #else
    #include <cairo-xlib.h>
  #endif
#endif


#ifdef _WIN32
  #include <windows.h>
  #define DIR_SEPARATOR '\\'
#else
  #define DIR_SEPARATOR '/'
#endif


#define EAST 1
#define WEST 2
#define NORTH 3
#define SOUTH 4

/* distance space from border to the ardesia bar in pixel unit */
#define SPACE_FROM_BORDER 25

#define COLORSIZE 9


#ifdef _WIN32
  #define UI_FILE "ardesia.glade"
  #define UI_HOR_FILE "ardesia_horizontal.glade"
#else
  #define UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/ardesia.glade"
  #define UI_HOR_FILE PACKAGE_DATA_DIR"/ardesia/ui/ardesia_horizontal.glade"
#endif  


typedef struct
{
  char* backgroundimage;
  gboolean debug;
  gboolean decorated;
  int position;
} CommandLine;

 typedef struct
 {
   /* annotation is visible */
   gboolean annotation_is_visible;

   /* pencil is selected */
   gboolean pencil;

   /* grab when leave */
   gboolean grab;

   /* Is the text inserction enabled */
   gboolean text;

   /* selected color in RGBA format */
   gchar* color;

   /* selected line width */
   int thickness;

   /* highlighter flag */
   gboolean highlighter;

   /* rectifier flag */
   gboolean rectifier;

   /* rounder flag */
   gboolean rounder;

   /* arrow flag */
   gboolean arrow;

   /* Default folder where store images and videos */
   char* workspace_dir;
   
 }BarData;
 
 
 
