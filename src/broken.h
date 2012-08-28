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


#ifndef BROKEN_FILE
#define BROKEN_FILE


/* Return a new list containing a sub-path of list_inp that contains
 * the meaningful points using the standard deviation algorithm.
 */
GSList *
build_meaningful_point_list     (GSList   *list_inp,
                                 gboolean  rectify,
                                 gdouble   pixel_tollerance);


/* Return a new out-bounded rectangle outside the path described to list_in. */
GSList *
build_outbounded_rectangle (GSList *list);


/* Is the path similar to an ellipse;
 * unbounded_rect is the out-bounded rectangle to the shape. */
gboolean
is_similar_to_an_ellipse        (GSList   *list,
                                 gdouble   pixel_tollerance);


/* Take a list of point and return magically the new recognized path. */
GSList *
broken                          (GSList   *inp,
                                 gboolean  close_path,
                                 gboolean  rectify,
                                 gdouble   pixel_tollerance);

#endif


