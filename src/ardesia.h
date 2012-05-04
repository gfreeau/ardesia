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


#include <config.h>
#include <locale.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <getopt.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>


/* The position used to localize the bar. */
#define EAST 1
#define WEST 2
#define NORTH 3
#define SOUTH 4


/* The structure that contains the command line info. */
typedef struct
{

  /* The file name of the iwb. */
  gchar *iwb_filename;

  /* Is the debug mode enabled? */
  gboolean debug;
  
  /* Is the bar windows decorated? */
  gboolean decorated;

  /* Where is located the ardesia bar? */
  gint position;

  /* Options for text_window */
  gchar *fontfamily;
  gint text_leftmargin;
  gint text_tabsize;

} CommandLine;


