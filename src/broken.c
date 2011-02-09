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


#include <utils.h>
#include <annotation_window.h>
#include <broken.h>


/* Number x is roundable to y. */
static gboolean is_similar(gdouble x, gdouble y, gdouble pixel_tollerance)
{
  gdouble delta = abs(x-y); 
  if (delta <= pixel_tollerance) 
    {
      return TRUE;
    }
  return FALSE;
}


/* 
 * The list of point is roundable to a rectangle
 * Note this algorithm found only the rettangle parallel to the axis.
 */
static gboolean is_a_rectangle(GSList* list, gdouble pixel_tollerance)
{
  AnnotateStrokeCoordinate* point0 = NULL;
  AnnotateStrokeCoordinate* point1 = NULL;
  AnnotateStrokeCoordinate* point2 = NULL;
  AnnotateStrokeCoordinate* point3 = NULL;
  
  if ( g_slist_length(list) != 4)
    {
      return FALSE; 
    }
  
  point0 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 0);
  point1 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 1);

  if (!(is_similar(point0->x, point1->x, pixel_tollerance)))
    {
      return FALSE; 
    }

  point2 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 2);
  if (!(is_similar(point1->y, point2->y, pixel_tollerance)))
    {
      return FALSE; 
    }
  
  point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 3);
  if (!(is_similar(point2->x, point3->x, pixel_tollerance)))
    {
      return FALSE; 
    }
  if (!(is_similar(point3->y, point0->y, pixel_tollerance)))
    {
      return FALSE; 
    }

  /* Postcondition: it is a rectangle. */
  return TRUE;
}


/* Calculate the media of the point pression. */
static gdouble calculate_medium_pression(GSList *list)
{
  gint i = 0;
  gdouble total_pressure = 0;
  while (list)
    {
      AnnotateStrokeCoordinate* cur_point = (AnnotateStrokeCoordinate*)list->data;
      total_pressure = total_pressure + cur_point->pressure;
      i = i +1;
      list=list->next;
    }
  return total_pressure/i;
}


/* Take the list and the rurn the minx miny maxx and maxy points. */
static gboolean found_min_and_max(GSList* list, 
				  gdouble* minx, 
				  gdouble* miny, 
				  gdouble* maxx, 
				  gdouble* maxy, 
				  gdouble* total_pressure)
{
  gint lenght = 0;
  AnnotateStrokeCoordinate* out_point = NULL;
  
  /* This function has sense only for the list with lenght greater than two. */
  lenght = g_slist_length(list);
  if (lenght < 3)
    {
      return FALSE;
    }
  out_point = (AnnotateStrokeCoordinate*)list->data;
  *minx = out_point->x;
  *miny = out_point->y;
  *maxx = out_point->x;
  *maxy = out_point->y;
  *total_pressure = out_point->pressure;
  
  while (list)
    {
      AnnotateStrokeCoordinate* cur_point = (AnnotateStrokeCoordinate*)list->data;
      *minx = MIN(*minx, cur_point->x);
      *miny = MIN(*miny, cur_point->y);
      *maxx = MAX(*maxx, cur_point->x);
      *maxy = MAX(*maxy, cur_point->y);
      *total_pressure = *total_pressure + cur_point->pressure;
      list = list->next; 
    }   
  return TRUE;
}


/* The path described in list is similar to a regular poligon. */
static gboolean is_similar_to_a_regular_poligon(GSList* list, gdouble pixel_tollerance)
{
  gint i = 0;
  gdouble ideal_distance = -1;
  gdouble total_distance = 0;  
  
  gint lenght = g_slist_length(list);
  AnnotateStrokeCoordinate* old_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
              
  for (i=1; i<lenght; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      gdouble distance = get_distance(old_point->x, old_point->y, point->x, point->y);
      total_distance = total_distance + distance;
      old_point = point;
    }

  ideal_distance = total_distance/lenght;
  
  // printf("Ideal %f\n\n",ideal_distance);

  i = 0;
  old_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
  for (i=1; i<lenght; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      /* I have seen that a good compromise allow around 33% of error. */
      gdouble threshold =  ideal_distance/3 + pixel_tollerance;
      gdouble distance = get_distance(point->x, point->y, old_point->x, old_point->y);
      if (!(is_similar(distance, ideal_distance, threshold)))
	{
	  return FALSE; 
	}
      old_point = point;
    }
  return TRUE;
}


/* Take a path and return the regular poligon path. */
static GSList* extract_poligon(GSList* list_in)
{
  gdouble cx, cy;
  gdouble radius;
  gdouble minx;
  gdouble miny;
  gdouble maxx;
  gdouble maxy;
  gdouble total_pressure;
  gboolean status = FALSE;  
  gdouble angle_off = M_PI/2;
  gdouble x1, y1;
  gint i;
  gint lenght = 0;
  gdouble angle_step = 0;
  gdouble medium_pressure = 1.0;
  AnnotateStrokeCoordinate* last_point = NULL; 
  AnnotateStrokeCoordinate* first_point = NULL; 

  status = found_min_and_max(list_in, &minx, &miny, &maxx, &maxy, &total_pressure);
  
  if (!status)
    {
      return NULL;
    }

  cx = (maxx + minx)/2;   
  cy = (maxy + miny)/2;   
  radius = ((maxx-minx)+(maxy-miny))/4;   
  lenght = g_slist_length(list_in);
  angle_step = 2 * M_PI / (lenght-1);
  angle_off += angle_step/2;
  medium_pressure = total_pressure/lenght;
  
  for (i=0; i<lenght-1; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_in, i);
      x1 = radius * cos(angle_off) + cx;
      y1 = radius * sin(angle_off) + cy;
      point->x = x1;
      point->y = y1;
      point->pressure = medium_pressure;
      angle_off += angle_step;
    }

  last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_in, lenght -1); 
  first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_in, 0); 
  last_point->x = first_point->x;
  last_point->y = first_point->y;
  last_point->pressure = medium_pressure;

  return list_in;  
}


/* Set x-axis of the point. */
void putx(gpointer current, gpointer value)
{
  AnnotateStrokeCoordinate* current_point = ( AnnotateStrokeCoordinate* ) current;
  gdouble* valuex = ( gdouble* ) value;
  current_point->x = *valuex;
}


/* Set y-axis of the point. */
static void puty(gpointer current, gpointer value)
{
  AnnotateStrokeCoordinate* current_point = ( AnnotateStrokeCoordinate* ) current;
  gdouble* valuey = ( gdouble* ) value;
  current_point->y = *valuey;
}


/* Return the degree of the rect beetween two point respect the axis. */
static gfloat calculate_edge_degree(AnnotateStrokeCoordinate* point_a, 
				    AnnotateStrokeCoordinate* point_b)
{
  gdouble deltax = abs(point_a->x-point_b->x);
  gdouble deltay = abs(point_a->y-point_b->y);
  gfloat direction_ab = atan2(deltay, deltax)/M_PI*180;
  return direction_ab;   
}


/* Straight the line. */
static GSList* straighten(GSList* list)
{  
  gint degree_threshold = 15;
  GSList* list_out = NULL;
  gint lenght = 0;
  AnnotateStrokeCoordinate* inp_point = NULL;
  AnnotateStrokeCoordinate* first_point =  NULL;
  AnnotateStrokeCoordinate* last_point = NULL;  
  AnnotateStrokeCoordinate* last_out_point =  NULL;
  gint i;
  gfloat direction;  
  
  lenght = g_slist_length(list);
  
  /* Copy the first one point; it is a good point. */
  inp_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 0);
  first_point =  allocate_point(inp_point->x, inp_point->y, inp_point->width, inp_point->pressure);
  list_out = g_slist_prepend (list_out, first_point); 
 
  for (i=0; i<lenght-2; i++)
    {
      AnnotateStrokeCoordinate* point_a = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      AnnotateStrokeCoordinate* point_b = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i+1);
      AnnotateStrokeCoordinate* point_c = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i+2);
      gfloat direction_ab = calculate_edge_degree(point_a, point_b);  
      gfloat direction_bc = calculate_edge_degree(point_b, point_c);  
      gfloat delta_degree = abs(direction_ab-direction_bc);

      if (delta_degree > degree_threshold)
        {
	  /* Copy B it's a good point. */
	  AnnotateStrokeCoordinate* point =  allocate_point(point_b->x, 
							    point_b->y, 
							    point_b->width, 
							    point_b->pressure);

	  list_out = g_slist_prepend (list_out, point); 
        } 
      /* Else: is three the difference degree is minor than the threshold I neglegt B. */
    }

  /* Copy the last point; it is a good point. */
  last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, lenght-1);  
  last_out_point =  allocate_point(last_point->x, 
				   last_point->y, 
				   last_point->width, 
				   last_point->pressure);

  list_out = g_slist_prepend (list_out, last_out_point);

  /* I reverse the list to preserve the initial order. */
  list_out = g_slist_reverse(list_out);

  lenght = g_slist_length(list_out);
  
  if (lenght!=2)  
    {
      return list_out;
    }
  
  /* It is a segment! */   
  direction = calculate_edge_degree(first_point, last_point);  

  /* is it is closed to 0 degree I draw an horizontal line. */
  if ((0-degree_threshold<=direction)&&(direction<=0+degree_threshold)) 
    {
      /* y is the average */
      gdouble y = (first_point->y+last_point->y)/2;
      /* Put this y for each element in the list. */
      g_slist_foreach(list_out, (GFunc)puty, &y);
    } 
  
  /* It is closed to 90 degree I draw a vertical line. */
  if ((90-degree_threshold<=direction)&&(direction<=90+degree_threshold)) 
    {
      /* x is the average */
      gdouble x = (first_point->x+last_point->x)/2;
      /* put this x for each element in the list. */
      g_slist_foreach(list_out, (GFunc)putx, &x);
    } 

  return list_out; 
}


/* Return a subpath of list_inp containg only the meaningful points 
 * using the standard deviation algorithm. 
 */
GSList* extract_relevant_points(GSList *list_inp, gboolean close_path, gdouble pixel_tollerance)
{
  gint lenght = g_slist_length(list_inp);
  /* Initialize the list. */
  GSList* list_out = NULL;
  
  gint i = 0;
  AnnotateStrokeCoordinate* point_a = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_inp, i);
  AnnotateStrokeCoordinate* point_b = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_inp, i+1);
  AnnotateStrokeCoordinate* point_c = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_inp, lenght-1);

  gdouble pressure = calculate_medium_pression(list_inp);

  gdouble a_x = point_a->x;
  gdouble a_y = point_a->y;
  gint a_width = point_a->width;

  gdouble b_x = point_b->x;
  gdouble b_y = point_b->y;
  gint b_width = point_b->width;

  gdouble c_x = point_b->x;
  gdouble c_y = point_b->y;
  gint c_width = point_b->width;
  
  gdouble area = 0.;
  gdouble h = 0;
  
  gdouble x1, y1, x2, y2;

  AnnotateStrokeCoordinate* first_point =  allocate_point(a_x, a_y, a_width, pressure);
  AnnotateStrokeCoordinate* last_point =  NULL;
  
  /* add a point with the coordinates of point_a. */
  list_out = g_slist_prepend (list_out, first_point);

  for (i = i+2; i<lenght; i++)
    {
      point_c = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_inp, i);
      c_x = point_c->x;
      c_y = point_c->y;
      c_width = point_c->width;
       
      x1 = b_x - a_x;
      y1 = b_y - a_y;
      x2 = c_x - a_x;
      y2 = c_y - a_y;
    
      area += (gdouble) (x1 * y2 - x2 * y1);
    
      h = (2*area)/sqrtf(x2*x2 + y2*y2);    
       
      if (fabs(h) >= (pixel_tollerance))
	{
	  /* Add  a point with the B coordinates. */
	  AnnotateStrokeCoordinate* new_point =  allocate_point(b_x, b_y, b_width, pressure);
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
  last_point =  allocate_point(c_x, c_y, c_width, pressure);
  list_out = g_slist_prepend (list_out, last_point); 

  /* I reverse the list to preserve the initial order. */
  list_out = g_slist_reverse(list_out);
  return list_out;
}


/* Return the outbounded rectangle outside the path described to list_in. */
GSList*  extract_outbounded_rectangle(GSList* list_in)
{ 
  gdouble minx;
  gdouble miny;
  gdouble maxx;
  gdouble maxy;
  gdouble total_pressure = 1.0;
  gint lenght = g_slist_length(list_in);
  GSList* list_out = NULL;
  AnnotateStrokeCoordinate* point0 = NULL;
  AnnotateStrokeCoordinate* point1 = NULL;
  AnnotateStrokeCoordinate* point2 = NULL;
  AnnotateStrokeCoordinate* point3 = NULL;
  gdouble media_pressure = 1.0; 
 
  AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_in, 0);
  gint width = first_point->width;

  gboolean status = found_min_and_max(list_in, &minx, &miny, &maxx, &maxy, &total_pressure);
  if (!status)
    {
      return NULL;
    }

  media_pressure = total_pressure/lenght; 

  point0 = allocate_point (minx, miny, width, media_pressure);
  list_out = g_slist_prepend (list_out, point0);
                       
  point1 =  allocate_point (maxx, miny, width, media_pressure);
  list_out = g_slist_prepend (list_out, point1);

  point2 =  allocate_point (maxx, maxy, width, media_pressure);
  list_out = g_slist_prepend (list_out, point2);

  point3 =  allocate_point (minx, maxy, width, media_pressure);
  list_out = g_slist_prepend (list_out, point3);

  return list_out;
}

       
/* The path described in list is similar to an ellipse,  unbounded_rect is the outbounded rectangle to the eclipse. */
gboolean is_similar_to_an_ellipse(GSList* list, GSList* unbounded_rect, gdouble pixel_tollerance)
{
  gint lenght = g_slist_length(list);
  gint i=0;   
   
  AnnotateStrokeCoordinate* point1 = (AnnotateStrokeCoordinate*) g_slist_nth_data (unbounded_rect, 2);
  AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (unbounded_rect, 0);

  gdouble a = (point3->x-point1->x)/2;
  gdouble b = (point3->y-point1->y)/2;
  gdouble originx = point1->x + a;
  gdouble originy = point1->y + b;
  gdouble c = 0.0;
  gdouble f1x;
  gdouble f1y;
  gdouble f2x;
  gdouble f2y;

  gdouble aq = powf(a,2);
  gdouble bq = powf(b,2);
  
  gdouble distance_p1f1;
  gdouble distance_p1f2;
  gdouble sump1;
  
  if (aq>bq)
    {
      c = sqrtf(aq-bq);
      // F1(x0-c,y0)
      f1x = originx-c;
      f1y = originy;
      // F2(x0+c,y0)
      f2x = originx+c;
      f2y = originy;
    }
  else
    {
      c = sqrtf(bq-aq);
      // F1(x0, y0-c)
      f1x = originx;
      f1y = originy-c;
      // F2(x0, y0+c)
      f2x = originx;
      f2y = originy+c;
    }

  distance_p1f1 = get_distance(point1->x, point1->y, f1x, f1y);
  distance_p1f2 = get_distance(point1->x, point1->y, f2x, f2y);
  sump1 = distance_p1f1 + distance_p1f2;

  /* In the ellipse the sum of the distance(p,f1)+distance(p,f2) must be constant. */

  for (i=0; i<lenght; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      gdouble distancef1 = get_distance(point->x, point->y, f1x, f1y);
      gdouble distancef2 = get_distance(point->x, point->y, f2x, f2y);
      gdouble sum = distancef1 + distancef2;
      gdouble difference = fabs(sum-sump1);
      if (difference>pixel_tollerance)
        {  
	  /* The sum is so different from the right one; this can not be approximated to an ellipse. */ 
	  return FALSE;
        }
    }
  return TRUE;
}

            
/* Take a list of point and return magically the new recognized path. */
GSList* broken(GSList* list_inp, gboolean close_path, gboolean rectify, gdouble pixel_tollerance)
{
  GSList* relevant_list = extract_relevant_points(list_inp, close_path, pixel_tollerance);  
   
  if (relevant_list)
    { 
      if (rectify) 
	{     
	  if (close_path)
	    {
	      /* It is similar to regular a poligon. */ 
	      if (is_similar_to_a_regular_poligon(relevant_list, pixel_tollerance))
		{
		  relevant_list = extract_poligon(relevant_list);
		} 
	      else
		{

		  if (is_a_rectangle(relevant_list, pixel_tollerance))
		    {
		      /* It is a rectangle. */
		      GSList* ret_list = extract_outbounded_rectangle(relevant_list);
		      g_slist_foreach(relevant_list, (GFunc)g_free, NULL);
		      g_slist_free(relevant_list);
		      return ret_list;
		    }
		}
	    }
	  else
	    {
	      /* Try to make straighten. */
	      GSList* ret_list = straighten(relevant_list);
	      /* Free outptr. */
	      g_slist_foreach(relevant_list, (GFunc)g_free, NULL);
	      g_slist_free(relevant_list);
              return ret_list;
	    }
	} 
      else       
	{
	  return relevant_list;    
	}  
    }
  return relevant_list;
}


