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


/* Background window */
extern GtkWidget* background_window;

/* Save pixbuf in png file */
gboolean save_png (GdkPixbuf *pixbuf, const char *filename);

/* Load png file contents in pixbuf */
gboolean load_png (const char *name, GdkPixbuf **pixmap);

/* Make screenshot */
void makeScreenshot(char* filename);

/* Change the background image */
void change_background_image(const char *filename);

/* Change the background color */
void change_background_color(char *rgb);

/* Remove the background */
void remove_background();

