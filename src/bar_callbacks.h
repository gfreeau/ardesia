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

/* full opaque alpha */
#define OPAQUE_ALPHA "FF"

/* Semi opaque (and then semi transparent) alpha;
 * this is used to make the highlighter effect.
 */
#define SEMI_OPAQUE_ALPHA "66"

/* The time-out after that the tool try to up-rise the window;
 * this is done to prevent the window lowering in the case that
 * the window manager does not support the stay above directive.
 */
#define  BAR_TO_TOP_TIMEOUT 1000


