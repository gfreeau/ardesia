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


#include <glib.h>


#ifndef BROKEN_FILE
#define BROKEN_FILE

/* Return a subpath of listInp containg only the meaningful points using the standard deviation */
GSList* extract_relevant_points(GSList *listInp, gboolean rectify, gdouble pixel_tollerance);

/* Return the outbounded rectangle outside the path described to listIn */
GSList*  extract_outbounded_rectangle(GSList* listIn);

/* The path described in list is similar to an ellipse,  unbounded_rect is the outbounded rectangle to the eclipse */
gboolean is_similar_to_an_ellipse(GSList* list, GSList* unbounded_rect, gdouble pixel_tollerance); 

/* Take a list of point and return magically the new recognized path */        
GSList*    broken( GSList* inp, gboolean close_path, gboolean rectify, gdouble pixel_tollerance);

#endif


