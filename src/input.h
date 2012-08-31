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
 

#include <annotation_window.h>
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


/* Un-grab pointer. */
void
ungrab_pointer     (GdkDisplay         *display);


/* Grab pointer. */
void
grab_pointer       (GtkWidget            *win,
                    GdkEventMask          eventmask);


/* Remove all the devices . */
void
remove_input_devices    (AnnotateData *data);

/* Set-up input devices. */
void
setup_input_devices     (AnnotateData  *data);


/* Add input device. */
void
add_input_device   (AnnotateData     *data,
                    GdkDevice        *device);


/* Remove input device. */
void
remove_input_device     (AnnotateData  *data,
                         GdkDevice     *device);


