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


#include <gtk/gtk.h>

#include <cairo.h>

#include <glib.h>
#include <glib/gstdio.h>


#ifdef _WIN32
#  include <cairo-win32.h>
#  include <winuser.h>
#else
#  ifdef __APPLE__
#    include <cairo-quartz.h>
#  else
#    include <cairo-xlib.h>
#  endif
#endif


/* Confirm to override file dialog. */
gboolean
show_override_dialog (GtkWindow *parent_window);


/* Show the could not write the file */
void
show_could_not_write_dialog (GtkWindow *parent_window);


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot.
 */
void
start_save_image_dialog (GtkToolButton *toolbutton,
			 GtkWindow *parent);


