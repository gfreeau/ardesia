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


#include <utils.h>
#include <annotation_window.h>
#include <broken.h>


/* Number x is roundable to y. */
static gboolean
is_similar (gdouble x,
            gdouble y,
            gdouble pixel_tollerance)
{
  gdouble delta = fabs (x-y);

  if (delta <= pixel_tollerance)
    {
      return TRUE;
    }

  return FALSE;
}


/* 
 * The list of point is roundable to a rectangle
 * Note this algorithm found only the rectangle parallel to the axis.
 */
static gboolean
is_a_rectangle (GSList *list,
                gdouble pixel_tollerance)
{

  if ( g_slist_length (list) != 4)
    {
      return FALSE;
    }
  else
    {
      AnnotatePoint *point0 = (AnnotatePoint *) g_slist_nth_data (list, 0);
      AnnotatePoint *point1 = (AnnotatePoint *) g_slist_nth_data (list, 1);
      AnnotatePoint *point2 = (AnnotatePoint *) g_slist_nth_data (list, 2);
      AnnotatePoint *point3 = (AnnotatePoint *) g_slist_nth_data (list, 3);

      if (! (is_similar (point0->x, point1->x, pixel_tollerance) ))
        {
          return FALSE;
        }

      if (! (is_similar (point1->y, point2->y, pixel_tollerance)))
        {
          return FALSE;
        }

      if (! (is_similar (point2->x, point3->x, pixel_tollerance)))
        {
          return FALSE;
        }

      if (! (is_similar (point3->y, point0->y, pixel_tollerance)))
        {
          return FALSE;
        }
    }

  /* Postcondition: it is a rectangle. */
  return TRUE;
}


/* Calculate the media of the point pression. */
static gdouble
calculate_medium_pression (GSList *list)
{
  guint i = 0;
  gdouble total_pressure = 0;
  guint lenght = g_slist_length (list);

  for (i=0; i<lenght; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      total_pressure = total_pressure + cur_point->pressure;
    }

  return total_pressure/i;
}


/* Take the list and found the minx miny maxx and maxy points. */
static void found_min_and_max (GSList  *list,
                               gdouble *minx,
                               gdouble *miny,
                               gdouble *maxx,
                               gdouble *maxy)
{
  guint i = 0;

  /* Initialize the min and max to the first point coordinates */
  AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (list, i);
  *minx = first_point->x;
  *miny = first_point->y;
  *maxx = first_point->x;
  *maxy = first_point->y;

  guint lenght = g_slist_length (list);

  /* Search the min and max coordinates */
  for (i=1; i<lenght; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      *minx = MIN (*minx, cur_point->x);
      *miny = MIN (*miny, cur_point->y);
      *maxx = MAX (*maxx, cur_point->x);
      *maxy = MAX (*maxy, cur_point->y);
    }
}


/* The path described in list is similar to a regular polygon. */
static gboolean
is_similar_to_a_regular_polygon (GSList *list,
                                 gdouble pixel_tollerance)
{
  guint i = 0;
  gdouble ideal_distance = -1;
  gdouble total_distance = 0;

  guint lenght = g_slist_length (list);
  AnnotatePoint *old_point = (AnnotatePoint *) g_slist_nth_data (list, i);

  for (i=1; i<lenght; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
      gdouble distance = get_distance (old_point->x, old_point->y, point->x, point->y);
      total_distance = total_distance + distance;
      old_point = point;
    }

  ideal_distance = total_distance/lenght;

  // printf ("Ideal %f\n\n",ideal_distance);

  i = 0;
  old_point = (AnnotatePoint *) g_slist_nth_data (list, i);

  for (i=1; i<lenght; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
      /* I have seen that a good compromise allow around 33% of error. */
      gdouble threshold =  ideal_distance/3 + pixel_tollerance;
      gdouble distance = get_distance (point->x, point->y, old_point->x, old_point->y);

      if (! (is_similar (distance, ideal_distance, threshold)))
        {
          return FALSE;
        }

      old_point = point;
    }

  return TRUE;
}


/* Take a path and return the regular polygon path. */
static GSList *
extract_polygon (GSList *list)
{
  gdouble cx = -1;
  gdouble cy = -1;
  gdouble radius = -1;
  gdouble minx = -1;
  gdouble miny = -1;
  gdouble maxx = -1;
  gdouble maxy = -1;
  gdouble angle_off = M_PI/2;
  gdouble x1 = -1;
  gdouble y1 = -1;
  guint i = 0;
  guint lenght = 0;
  gdouble angle_step = 0;
  AnnotatePoint *last_point = NULL;
  AnnotatePoint *first_point = NULL;

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  cx = (maxx + minx)/2;
  cy = (maxy + miny)/2;
  radius = ((maxx-minx)+ (maxy-miny))/4;
  lenght = g_slist_length (list);
  angle_step = 2 * M_PI / (lenght-1);
  angle_off += angle_step/2;

  for (i=0; i<lenght-1; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
      x1 = radius * cos (angle_off) + cx;
      y1 = radius * sin (angle_off) + cy;
      point->x = x1;
      point->y = y1;
      angle_off += angle_step;
    }

  last_point = (AnnotatePoint *) g_slist_nth_data (list, lenght -1);
  first_point = (AnnotatePoint *) g_slist_nth_data (list, 0);
  last_point->x = first_point->x;
  last_point->y = first_point->y;

  return list;
}


/* Set x-axis of the point. */
static void
point_put_x (gpointer current,
             gpointer value)
{
  AnnotatePoint *current_point = (AnnotatePoint *) current;
  gdouble *valuex = (gdouble *) value;
  current_point->x = *valuex;
}


/* Set y-axis of the point. */
static void
point_put_y (gpointer current,
             gpointer value)
{
  AnnotatePoint *current_point = ( AnnotatePoint *) current;
  gdouble *value_y = (gdouble *) value;
  current_point->y = *value_y;
}


/* Return the degree of the rectangle between two point respect the axis. */
static gdouble
calculate_edge_degree (AnnotatePoint *point_a,
                       AnnotatePoint *point_b)
{
  gdouble deltax = fabs (point_a->x-point_b->x);
  gdouble deltay = fabs (point_a->y-point_b->y);
  gdouble direction_ab = atan2 (deltay, deltax)/M_PI*180;
  return direction_ab;
}


/* Straight the line. */
static GSList *
straighten (GSList *list)
{
  AnnotatePoint *inp_point = NULL;
  AnnotatePoint *first_point =  NULL;
  AnnotatePoint *last_point = NULL;
  AnnotatePoint *last_out_point =  NULL;
  gdouble degree_threshold = 15;
  GSList *list_out = NULL;
  guint lenght = 0;
  guint i;
  gdouble direction;

  lenght = g_slist_length (list);
  
  /* Copy the first one point; it is a good point. */
  inp_point = (AnnotatePoint *) g_slist_nth_data (list, 0);
  first_point =  allocate_point (inp_point->x, inp_point->y, inp_point->width, inp_point->pressure);
  list_out = g_slist_prepend (list_out, first_point);

  for (i=0; i<lenght-2; i++)
    {
      AnnotatePoint *point_a = (AnnotatePoint *) g_slist_nth_data (list, i);
      AnnotatePoint *point_b = (AnnotatePoint *) g_slist_nth_data (list, i+1);
      AnnotatePoint *point_c = (AnnotatePoint *) g_slist_nth_data (list, i+2);
      gdouble direction_ab = calculate_edge_degree (point_a, point_b);
      gdouble direction_bc = calculate_edge_degree (point_b, point_c);
      gdouble delta_degree = fabs (direction_ab-direction_bc);

      if (delta_degree > degree_threshold)
        {
          /* Copy B it's a good point. */
          AnnotatePoint *point =  allocate_point (point_b->x,
                                                  point_b->y,
                                                  point_b->width,point_b->pressure);
                                                  
          list_out = g_slist_prepend (list_out, point);
        }

      /* Else: is three the difference degree is minor than the threshold I neglegt B. */
    }

  /* Copy the last point; it is a good point. */
  last_point = (AnnotatePoint *) g_slist_nth_data (list, lenght-1);
  last_out_point =  allocate_point (last_point->x,
                                    last_point->y,
                                    last_point->width,
                                    last_point->pressure);

  list_out = g_slist_prepend (list_out, last_out_point);

  /* I reverse the list to preserve the initial order. */
  list_out = g_slist_reverse (list_out);

  lenght = g_slist_length (list_out);

  if (lenght!=2)
    {
      return list_out;
    }

  /* It is a segment! */
  direction = calculate_edge_degree (first_point, last_point);

  /* is it is closed to 0 degree I draw an horizontal line. */
  if ( (0-degree_threshold<=direction) && (direction<=0+degree_threshold) )
    {
      /* y is the average */
      gdouble y = (first_point->y+last_point->y)/2;
      /* Put this y for each element in the list. */
      g_slist_foreach (list_out, (GFunc) point_put_y, &y);
    }

  /* It is closed to 90 degree I draw a vertical line. */
  if ( (90-degree_threshold<=direction)&& (direction<=90+degree_threshold))
    {
      /* x is the average */
      gdouble x = (first_point->x+last_point->x)/2;
      /* put this x for each element in the list. */
      g_slist_foreach (list_out, (GFunc) point_put_x, &x);
    }

  return list_out;
}


/* Return a new list containing a sub-path of list_inp that contains
 * the meaningful points using the standard deviation algorithm.
 */
GSList *
build_meaningful_point_list     (GSList *list_inp,
                                 gboolean rectify,
                                 gdouble pixel_tollerance)
{
  guint lenght = g_slist_length (list_inp);
  guint i = 0;
  AnnotatePoint *point_a = (AnnotatePoint *) g_slist_nth_data (list_inp, i);
  AnnotatePoint *point_b = (AnnotatePoint *) g_slist_nth_data (list_inp, i+1);
  AnnotatePoint *point_c = NULL;
  gdouble pressure = calculate_medium_pression (list_inp);

  gdouble a_x = point_a->x;
  gdouble a_y = point_a->y;
  gdouble a_width = point_a->width;

  gdouble b_x = point_b->x;
  gdouble b_y = point_b->y;
  gdouble b_width = point_b->width;

  gdouble c_x = point_b->x;
  gdouble c_y = point_b->y;
  gdouble c_width = point_b->width;

  /* Initialize the list. */
  GSList *list_out = (GSList *) NULL;

  AnnotatePoint *first_point =  allocate_point (a_x, a_y, a_width, pressure);
  /* add a point with the coordinates of point_a. */
  list_out = g_slist_prepend (list_out, first_point);

  if (lenght == 2)
    {
      AnnotatePoint *second_point =  allocate_point (b_x, b_y, b_width, pressure);
      /* add a point with the coordinates of point_a. */
      list_out = g_slist_prepend (list_out, second_point);
    }
  else
    {
      gdouble area = 0.0;
      gdouble h = 0.0;
      gdouble x1 = 0.0;
      gdouble y1 = 0.0;
      gdouble x2 = 0.0;
      gdouble y2 = 0.0;
      AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (list_inp, lenght-1);
      AnnotatePoint *last_point_copy = (AnnotatePoint *) NULL;

      for (i = i+2; i<lenght; i++)
	{
	  point_c = (AnnotatePoint *) g_slist_nth_data (list_inp, i);
	  c_x = point_c->x;
	  c_y = point_c->y;
	  c_width = point_c->width;

	  x1 = b_x - a_x;
	  y1 = b_y - a_y;
	  x2 = c_x - a_x;
	  y2 = c_y - a_y;

	  area += (gdouble) (x1 * y2 - x2 * y1);

	  h = (2*area)/sqrt (x2*x2 + y2*y2);

	  if (fabs (h) >= (pixel_tollerance))
	    {
	      /* Add  a point with the B coordinates. */
	      AnnotatePoint *new_point =  allocate_point (b_x, b_y, b_width, pressure);
	      list_out = g_slist_prepend (list_out, new_point);
	      area = 0.0;
	      a_x = b_x;
	      a_y = b_y;
	      a_width = b_width;
	    }
	  /* Put to B the C coordinates. */
	  b_x = c_x;
	  b_y = c_y;
	  b_width = c_width;
	}

      /* Add the last point with the coordinates. */
      last_point_copy =  allocate_point (last_point->x,
                                         last_point->y,
                                         last_point->width,
                                         last_point->pressure);

      list_out = g_slist_prepend (list_out, last_point_copy);
    }

  /* I reverse the list to preserve the initial order. */
  list_out = g_slist_reverse (list_out);

  return list_out;
}


/* Return the out-bounded rectangle outside the path described to list_in. */
GSList *
build_outbounded_rectangle (GSList *list)
{
  guint lenght = g_slist_length (list);
  AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, lenght/2);
  GSList *ret_list = (GSList *) NULL;

  gdouble minx = 0;
  gdouble miny = 0;
  gdouble maxx = 0;
  gdouble maxy = 0;

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);


  AnnotatePoint *point3 =  allocate_point (minx, maxy, point->width, point->pressure);
  ret_list = g_slist_prepend (ret_list, point3);

  AnnotatePoint *point2 =  allocate_point (maxx, maxy, point->width, point->pressure);
  ret_list = g_slist_prepend (ret_list, point2);

  AnnotatePoint *point1 =  allocate_point (maxx, miny, point->width, point->pressure);
  ret_list = g_slist_prepend (ret_list, point1);

  AnnotatePoint *point0 =  allocate_point (minx, miny, point->width, point->pressure);

  ret_list = g_slist_prepend (ret_list, point0);

  return ret_list;
}


/* The path in list is similar to an ellipse. */
gboolean
is_similar_to_an_ellipse (GSList *list,
                          gdouble pixel_tollerance)
{
  guint i = 0;
  gdouble minx = 0;
  gdouble miny = 0;
  gdouble maxx = 0;
  gdouble maxy = 0;
  gdouble tollerance = 0;

  /* Semi x-axis */
  gdouble a = 0;

  /* Semi y-axis */
  gdouble b = 0;

  gdouble c = 0.0;

  /* x coordinate of the origin */
  gdouble originx = 0;

  /* y coordinate of the origin */
  gdouble originy = 0;

  /* x coordinate of focus1 */
  gdouble f1x = 0;

  /* y coordinate of focus1 */
  gdouble f1y = 0;

  /* x coordinate of focus2 */
  gdouble f2x = 0;

  /* y coordinate of focus2 */
  gdouble f2y = 0;

  gdouble distance_p1f1 = 0;
  gdouble distance_p1f2 = 0;
  gdouble sump1 = 0;

  gdouble aq = 0;
  gdouble bq = 0;

  guint lenght = g_slist_length (list);

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  a = (maxx-minx)/2;
  b = (maxy-miny)/2;

  aq = pow (a,2);
  bq = pow (b,2);

  /* 
   * If in one point the sum of the distance by focus F1 and F2 differer more than
   * the tolerance value the curve line will not be considered an ellipse.
   */
  tollerance = pixel_tollerance + (a + b) /2;

  originx = minx + a;
  originy = miny + b;

  if (aq>bq)
    {
      c = sqrt (aq-bq);
      // F1 (x0-c,y0)
      f1x = originx-c;
      f1y = originy;
      // F2 (x0+c,y0)
      f2x = originx+c;
      f2y = originy;
    }
  else
    {
      c = sqrt (bq-aq);
      // F1 (x0, y0-c)
      f1x = originx;
      f1y = originy-c;
      // F2 (x0, y0+c)
      f2x = originx;
      f2y = originy+c;
    }

  distance_p1f1 = get_distance (minx, miny, f1x, f1y);
  distance_p1f2 = get_distance (minx, miny, f2x, f2y);
  sump1 = distance_p1f1 + distance_p1f2;

  /* In the ellipse the sum of the distance (p,f1)+distance (p,f2) must be constant. */

  for (i=0; i<lenght; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
      gdouble distancef1 = get_distance (point->x, point->y, f1x, f1y);
      gdouble distancef2 = get_distance (point->x, point->y, f2x, f2y);
      gdouble sum = distancef1 + distancef2;
      gdouble difference = fabs (sum-sump1);

      if (difference>tollerance)
        {
          /* The sum is too different from the ideal one;
           * I do not approximate the shape to an ellipse.
           */
          return FALSE;
        }

    }

  return TRUE;
}


/* Return a list rectified */
GSList*
build_rectified_list(GSList  *list_inp,
                     gboolean close_path,
                     gdouble  pixel_tollerance)
{
  GSList *ret_list = (GSList *) NULL;
  if (close_path)
    {

      guint lenght = g_slist_length (list_inp);
      guint i = 0;

      /* Copy the input list */
      for (i=0; i<lenght; i++)
        {
          AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list_inp, i);
          AnnotatePoint *point_copy =  allocate_point (point->x, point->y, point->width, point->pressure);
          ret_list = g_slist_prepend (ret_list, point_copy);
        }

      /* I reverse the list to preserve the initial order. */
      ret_list = g_slist_reverse (ret_list);

      /* jump the algorithm and return the list as is */
      if (g_slist_length (ret_list) <= 3)
        {
          return ret_list;
        }

      /* It is similar to regular a polygon. */
      if (is_similar_to_a_regular_polygon (ret_list, pixel_tollerance))
        {
          ret_list = extract_polygon (ret_list);
        }
      else
        {
        
          if (is_a_rectangle (ret_list, pixel_tollerance))
            {
              /* It is a rectangle. */
              GSList *rect_list = build_outbounded_rectangle (ret_list);
              g_slist_foreach (ret_list, (GFunc)g_free, NULL);
              g_slist_free (ret_list);
              ret_list = rect_list;
            }
        }
    }
  else
    {
      /* Try to make straighten. */
      ret_list = straighten (list_inp);
    }

  return ret_list;
}


/* Take a list of point and return magically the new recognized path. */
GSList *
broken (GSList   *list_inp,
        gboolean  close_path,
        gboolean  rectify,
        gdouble   pixel_tollerance)
{
  GSList *meaningful_point_list = build_meaningful_point_list (list_inp, close_path, pixel_tollerance);

  if (meaningful_point_list)
    {

      if (rectify)
        {
          GSList * rectified_list = build_rectified_list(meaningful_point_list, close_path, pixel_tollerance);

          /* Free the meaningful_point_list. */
          g_slist_foreach (meaningful_point_list, (GFunc)g_free, NULL);
          g_slist_free (meaningful_point_list);

          return rectified_list;
        }

    }

  return meaningful_point_list;
}


