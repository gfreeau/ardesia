/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
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


/* Ungrab pointer */
void ungrab_pointer(GdkDisplay* display, GtkWidget *win);


/* Grab pointer */
void grab_pointer(GtkWidget *win, GdkEventMask eventmask);


/* Take a GdkColor and return the RGB string */
char* gdkcolor_to_rgba(GdkColor* gdkcolor);


/* Set the cairo surface color to the RGBA string */
void cairo_set_source_color_from_string( cairo_t * cr, char* color);


/* Set the cairo surface color to transparent */
void  cairo_set_transparent_color(cairo_t * cr);


/* Clear cairo context */
void clear_cairo_context(cairo_t* cr);


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel
 */
GdkColor* rgb_to_gdkcolor(char* rgb);


/* Get the current date and format in a printable format */
char* get_date();


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir);


/*
 * Get the desktop folder;
 * Now this function use gconf to found the folder,
 * this means that this rutine works properly only
 * with the gnome desktop environment
 * We can investigate how-to do this
 * in a desktop environment independant way
 */
const gchar* get_desktop_dir (void);
