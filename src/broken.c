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
#include <annotate.h>
#include <broken.h>


/* Number x is roundable to y */
gboolean is_similar(int x, int y, int pixel_tollerance)
{
  int delta = abs(x-y); 
  if (delta <= pixel_tollerance) 
    {
      return TRUE;
    }
  return FALSE;
}


/* The point (x1,y2) caan be rounded to (x2,y2) if the distance is minor that the pixel_tollerance */
gboolean is_similar_point(int x1, int y1, int x2, int y2, int pixel_tollerance)
{
  int distance = get_distance(x1, y1, x2, y2);
  if (distance < pixel_tollerance)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}


/* The list contain a point roundable to x y */
gboolean contain_similar_point(int x, int y, GSList* list, int pixel_tollerance)
{
  if (!list)
    {
      return FALSE;
    }
  int i;
  gint lenght = g_slist_length(list);
  for (i=0; i<lenght; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      
      if (is_similar_point(x, y, point->x, point->y, pixel_tollerance))
	{
	  return TRUE;
	}
    } 
  return FALSE;
} 


/* 
 * The list of point is roundable to a rectangle
 * Note this algorithm found only the rettangle parallel to the axis
 */
gboolean is_a_rectangle(GSList* list, int pixel_tollerance)
{
  if (!list)
    {
      return FALSE;
    }
  if ( g_slist_length(list) != 4)
    {
      return FALSE; 
    }
  AnnotateStrokeCoordinate* point0 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 0);
  AnnotateStrokeCoordinate* point1 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 1);
  if (!(is_similar(point0->x, point1->x, pixel_tollerance)))
    {
      return FALSE; 
    }
  AnnotateStrokeCoordinate* point2 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 2);
  if (!(is_similar(point1->y, point2->y, pixel_tollerance)))
    {
      return FALSE; 
    }
  AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 3);
  if (!(is_similar(point2->x, point3->x, pixel_tollerance)))
    {
      return FALSE; 
    }
  if (!(is_similar(point3->y, point0->y, pixel_tollerance)))
    {
      return FALSE; 
    }
  // is a rectangle
  return TRUE;
}


/* Return a subpath of listInp containg only the meaningful points using the standard deviation */
GSList* extract_relevant_points(GSList *listInp, gboolean close_path, int pixel_tollerance)
{
  if (!listInp)
  {
    return NULL;
  }

  gint lenght = g_slist_length(listInp);

  /* Initialize the list */
  GSList* listOut = NULL;
  
  int i = 0;
  AnnotateStrokeCoordinate* pointA = (AnnotateStrokeCoordinate*) g_slist_nth_data (listInp, i);
  AnnotateStrokeCoordinate* pointB = (AnnotateStrokeCoordinate*) g_slist_nth_data (listInp, i+1);
  AnnotateStrokeCoordinate* pointC = (AnnotateStrokeCoordinate*) g_slist_nth_data (listInp, lenght-1);

  int Ax = pointA->x;
  int Ay = pointA->y;
  int Awidth = pointA->width;
  gdouble Apressure = pointA->pressure;
  int Bx = pointB->x;
  int By = pointB->y;
  int Bwidth = pointB->width;
  gdouble Bpressure = pointB->pressure;
  int Cx = pointB->x;
  int Cy = pointB->y;
  int Cwidth = pointB->width;
  gdouble Cpressure = pointB->pressure;

  // add a point with the coordinates of pointA
  AnnotateStrokeCoordinate* first_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  first_point->x = Ax;
  first_point->y = Ay;
  first_point->width = Awidth;
  first_point->pressure = Apressure;
  listOut = g_slist_prepend (listOut, first_point);

  double area = 0.;
  double h = 0;
  
  int X1, Y1, X2, Y2;

  for (i = i+2; i<lenght; i++)
    {
       pointC = (AnnotateStrokeCoordinate*) g_slist_nth_data (listInp, i);
       Cx = pointC->x;
       Cy = pointC->y;
       Cwidth = pointC->width;
       Cpressure = pointC->pressure;

       X1 = Bx - Ax;
       Y1 = By - Ay;
       X2 = Cx - Ax;
       Y2 = Cy - Ay;
    
       area += (double) (X1 * Y2 - X2 * Y1);
    
       h = (2*area)/sqrt(X2*X2 + Y2*Y2);    
       
       if (fabs(h) >= pixel_tollerance)
         {
            // add  a point with the B coordinates
            AnnotateStrokeCoordinate* new_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
            new_point->x = Bx;
            new_point->y = By;
            new_point->width = Bwidth;
            new_point->pressure = Bpressure;
            listOut = g_slist_prepend (listOut, new_point);
            area = 0.0;
            Ax = Bx;
            Ay = By;
            Awidth = Bwidth;
            Apressure = Bpressure;
         } 
       // put to B the C coordinates
       Bx = Cx;
       By = Cy;
       Bwidth = Cwidth;
       Bpressure = Cpressure;
    }
  
  if (!close_path)
    {
      /* Add the last point with the coordinates */
      AnnotateStrokeCoordinate* last_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      last_point->x = Cx;
      last_point->y = Cy;
      last_point->width = Cwidth;
      last_point->pressure = Cpressure;
      listOut = g_slist_prepend (listOut, last_point); 
    }

  /* I reverse the list to preserve the initial order */
  listOut = g_slist_reverse(listOut);
  return listOut;
}


/* Take the list and the rurn the minx miny maxx and maxy points */
gboolean found_min_and_max(GSList* list, gint* minx, gint* miny, gint* maxx, gint* maxy, gdouble* total_pressure)
{
  if (!list)
    {
      return FALSE;
    }
  /* This function has sense only for the list with lenght greater than two */
  gint lenght = g_slist_length(list);
  if (lenght < 3)
    {
       return FALSE;
    }
  AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)list->data;
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


/* The path described in list is similar to a regular poligon */
gboolean is_similar_to_a_regular_poligon(GSList* list)
{
  if (!list)
    {
      return FALSE;
    }
  gint lenght = g_slist_length(list);
  int old_distance = -1;
  int i = 0;
  AnnotateStrokeCoordinate* old_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
              
  for (i=1; i<lenght; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      int distance = get_distance(point->x, point->y, old_point->x, old_point->y);
      if (old_distance != -1)
        {
          /* I have seen that a good compromise allow a 30% of error */
          int threshold =  old_distance/3;
          if (!(is_similar(distance, old_distance, threshold)))
            {
              return FALSE; 
            }
          // average distance
          old_distance = (distance + old_distance)/2;
        }
      else
        {
          old_distance = distance;
        }
      old_point = point;
    }
  return TRUE;
}


/* Take a path and return the regular poligon path */
GSList* extract_poligon(GSList* listIn)
{
  if (!listIn)
    {
      return NULL;
    }
  int cx, cy;
  int radius;
  gint minx;
  gint miny;
  gint maxx;
  gint maxy;
  gdouble total_pressure;
  gboolean status = found_min_and_max(listIn, &minx, &miny, &maxx, &maxy, &total_pressure);
  if (!status)
    {
      return NULL;
    }
  cx = (maxx + minx)/2;   
  cy = (maxy + miny)/2;   
  radius = ((maxx-minx)+(maxy-miny))/4;   
  double angle_off = M_PI/2;
  gint lenght = g_slist_length(listIn);
  double angle_step = 2 * M_PI / lenght;
  angle_off += angle_step/2;
  double x1, y1;
  int i;
  gdouble medium_pressure = total_pressure/lenght;
  for (i=0; i<lenght; i++)
    {
      x1 = radius * cos(angle_off) + cx;
      y1 = radius * sin(angle_off) + cy;
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (listIn, i);
      point->x = x1;
      point->y = y1;
      point->pressure = medium_pressure;
      angle_off += angle_step;
    }
  return listIn;  
}


/* Return the outbounded rectangle outside the path described to listIn */
GSList*  extract_outbounded_rectangle(GSList* listIn)
{ 
  if (!listIn)
    {
      return NULL;
    }
  gint minx;
  gint miny;
  gint maxx;
  gint maxy;
  gdouble total_pressure = 1.0;
  gint lenght = g_slist_length(listIn);
  gboolean status = found_min_and_max(listIn, &minx, &miny, &maxx, &maxy, &total_pressure);
  if (!status)
    {
      return NULL;
    }
  GSList* listOut = NULL;
  gdouble media_pressure = total_pressure/lenght; 
  AnnotateStrokeCoordinate* pointF = (AnnotateStrokeCoordinate*) g_slist_nth_data (listIn, 0);
  gint width = pointF->width;

  AnnotateStrokeCoordinate* point0 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  point0->x = minx;
  point0->y = miny;
  point0->width = width;
  point0->pressure = media_pressure;
  listOut = g_slist_prepend (listOut, point0);
                       
  AnnotateStrokeCoordinate* point1 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  point1->x = maxx;
  point1->y = miny;
  point1->width = width;
  point1->pressure = media_pressure;
  listOut = g_slist_prepend (listOut, point1);

  AnnotateStrokeCoordinate* point2 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  point2->x = maxx;
  point2->y = maxy;
  point2->width = width;
  point2->pressure = media_pressure;
  listOut = g_slist_prepend (listOut, point2);

  AnnotateStrokeCoordinate* point3 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  point3->x = minx;
  point3->y = maxy;
  point3->width = width;
  point3->pressure = media_pressure;
  listOut = g_slist_prepend (listOut, point3);
  return listOut;
}


/* Set x-axis of the point */
void putx(gpointer current, gpointer value)
{
  AnnotateStrokeCoordinate* currentpoint = ( AnnotateStrokeCoordinate* ) current;
  int* valuex = ( int* ) value;
  currentpoint->x = *valuex;
}


/* Set y-axis of the point */
void puty(gpointer current, gpointer value)
{
  AnnotateStrokeCoordinate* currentpoint = ( AnnotateStrokeCoordinate* ) current;
  int* valuey = ( int* ) value;
  currentpoint->y = *valuey;
}


/* Return the degree of the rect beetween two point respect the axis */
gfloat calculate_edge_degree(AnnotateStrokeCoordinate* pointA, AnnotateStrokeCoordinate* pointB)
{
  int deltax = abs(pointA->x-pointB->x);
  int deltay = abs(pointA->y-pointB->y);
  gfloat directionAB = atan2(deltay, deltax)/M_PI*180;
  return directionAB;   
}


/* Straight the line */
GSList* straighten(GSList* list)
{  
  GSList* listOut = NULL;
  if (!list)
    {
      return listOut;
    }
  
 
  gint lenght = g_slist_length(list);
  
  int degree_threshold = 15;

  /* copy the first one point; it is a good point */
  AnnotateStrokeCoordinate* inp_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 0);
  AnnotateStrokeCoordinate* first_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  first_point->x = inp_point->x;
  first_point->y = inp_point->y;
  first_point->width = inp_point->width;
  first_point->pressure = inp_point->pressure;
  listOut = g_slist_prepend (listOut, first_point); 
 
  int i;
  for (i=0; i<lenght-2; i++)
    {
      AnnotateStrokeCoordinate* pointA = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i);
      AnnotateStrokeCoordinate* pointB = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i+1);
      AnnotateStrokeCoordinate* pointC = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i+2);
      gfloat directionAB = calculate_edge_degree(pointA, pointB);  
      gfloat directionBC = calculate_edge_degree(pointB, pointC);  

      gfloat delta_degree = abs(directionAB-directionBC);
      if (delta_degree > degree_threshold)
        {
           //copy B it's a good point
           AnnotateStrokeCoordinate* point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
           point->x = pointB->x;
           point->y = pointB->y;
           point->width = pointB->width;
           point->pressure = pointB->pressure;
           listOut = g_slist_prepend (listOut, point); 
        } 
       /* else is three the difference degree is minor than the threshold I neglegt B */
    }

  /* Copy the last point; it is a good point */
  AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, lenght-1);
  
  AnnotateStrokeCoordinate* last_out_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  last_out_point->x = last_point->x;
  last_out_point->y = last_point->y;
  last_out_point->width = last_point->width;
  last_out_point->pressure = last_point->pressure;
  listOut = g_slist_prepend (listOut, last_out_point);

  /* I reverse the list to preserve the initial order */
  listOut = g_slist_reverse(listOut);


  lenght = g_slist_length(listOut);
  
  if (lenght!=2)  
    {
       return listOut;
    }
  
  /* It is a segment! */   
  gfloat direction = calculate_edge_degree(first_point, last_point);  

  /* is it is closed to 0 degree I draw an horizontal line */
  if ((0-degree_threshold<=direction)&&(direction<=0+degree_threshold)) 
    {
       // y is the average
       int y = (first_point->y+last_point->y)/2;
       // put this y for each element in the list
       g_slist_foreach(listOut, (GFunc)puty, &y);
    } 
  
  /* is it is closed to 90 degree I draw a vertical line */
  if ((90-degree_threshold<=direction)&&(direction<=90+degree_threshold)) 
    {
       // x is the average
       int x = (first_point->x+last_point->x)/2;
       // put this x for each element in the list
       g_slist_foreach(listOut, (GFunc)putx, &x);
    } 

 return listOut; 
}

                   
/* Take a list of point and return magically the new recognized path */
GSList* broken(GSList* listInp, gboolean close_path, gboolean rectify, int pixel_tollerance)
{
  if (!listInp)
    {
      return NULL;
    }
  
  GSList* listOut = extract_relevant_points(listInp, close_path, pixel_tollerance);  
   
 
  if (listOut)
    { 
        if (rectify) 
	  {
         
     
            if (close_path)
              {
	        // is similar to regular a poligon 
	        if (is_similar_to_a_regular_poligon(listOut))
                  {
		    listOut = extract_poligon(listOut);
                  } 
	        else
	          {

		     if (is_a_rectangle(listOut, pixel_tollerance))
		       {
		         /* is a rectangle */
		         GSList* listOutN = extract_outbounded_rectangle(listOut);
		         g_slist_foreach(listOut, (GFunc)g_free, NULL);
		         g_slist_free(listOut);
		         listOut = listOutN;
		         return listOut;
		       }
	          }
              }
            else
              {
                // try to make straighten
                GSList* straight_list = straighten(listOut);
                // free outptr
                g_slist_foreach(listOut, (GFunc)g_free, NULL);
                g_slist_free(listOut);
                listOut = straight_list;
              }
          } 
        else       
         {
            if (close_path)
              {
                 // roundify
                 /*  make the outbounded rectangle that will used to draw the ellipse */
                 int lenght = g_slist_length(listOut);
                 if (lenght>2)
                   {
                      // minimum three point is required
	              GSList* listOutN = extract_outbounded_rectangle(listOut);
	              g_slist_foreach(listOut, (GFunc)g_free, NULL);
	              g_slist_free(listOut);
	              listOut = listOutN;
                    }
                 return listOut;
              }
	  }  
    }
  return listOut;
}


