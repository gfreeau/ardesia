/* 
 * Ardesia-- a program for painting on the screen 
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



#include <math.h>
#include <glib.h>

#include <glib.h>
#include <gdk/gdkinput.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <annotate.h>
#include <broken.h>


 

GSList* broken(GSList* listInp, gboolean* close_path, gboolean rectify)
{
    
  int X1,X2,Y1,Y2;
  double  area;
  int Ax, Ay, Bx, By, Cx, Cy;
  int numpoint = 2;
    
  double H;
  double     tollerance = 15;
  
  *close_path = FALSE;
  GSList* listOut = NULL; 
  
  /*copy the first one point */
  AnnotateStrokeCoordinate* inp_point = (AnnotateStrokeCoordinate*)listInp->data;
  AnnotateStrokeCoordinate* first_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  first_point->x = inp_point->x;
  first_point->y = inp_point->y;
  first_point->width = inp_point->width;
  
  guint lenght = g_slist_length(listInp); 
  AnnotateStrokeCoordinate* end_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (listInp, lenght-1);


  if ((abs(end_point->x-first_point->x)<tollerance) &&(abs(end_point->y-first_point->y)<tollerance))
    {
      /* close path */
      *close_path = TRUE;
    } 

  listOut = g_slist_prepend (listOut, first_point); 
 
  area = 0.;
  Ax = inp_point->x;
  Ay = inp_point->y;

  listInp = listInp->next;   
  inp_point = (AnnotateStrokeCoordinate*)listInp->data;
  Bx = inp_point->x;
  By = inp_point->y;
  Cx = inp_point->x;
  Cy = inp_point->y;
  gint width = inp_point->width;
  
  listInp = listInp->next;   
 
  while (listInp)
    {
      inp_point = (AnnotateStrokeCoordinate*)listInp->data;
      Cx = inp_point->x;
      Cy = inp_point->y;
      X1 = Bx-Ax;
      Y1 = By-Ay;
      X2 = Cx-Ax;
      Y2 = Cy-Ay;
      area += (double)(X1*Y2 - X2*Y1);
     
      H = (2*area)/sqrt(X2*X2+Y2*Y2);
       
      if (abs(H) > tollerance)
	{   
	  numpoint ++;
	  Ax = Bx;
	  Ay = By;
          // Take a further point with standard deviation greater than the tollerance
	  inp_point = (AnnotateStrokeCoordinate*)listOut->data;
	  if (abs(Bx-inp_point->x)<tollerance)
	    {
	      Bx=inp_point->x;
	    }
	  if (abs(By-inp_point->y)<tollerance)
	    {
	      By=inp_point->y;
	    }
	  AnnotateStrokeCoordinate* add_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
	  add_point->x = Bx;
	  add_point->y = By;
	  add_point->width = width;
	  listOut = g_slist_prepend (listOut, add_point);
	  area = 0.;
	}
      Bx = Cx;
      By = Cy;
      listInp = listInp->next;   

    }
  
  if (*close_path)
    {
      //printf(" point %d \n", numpoint );
      /* close path */
      if ((numpoint != 5)&&(rectify))
	{
	  return listOut;
	}
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)listOut->data;
      gint minx = out_point->x;
      gint miny = out_point->y;
      gint maxx = out_point->x;
      gint maxy = out_point->y;
      GSList* savedListOut = listOut;
                       
      while (listOut)
	{
	  AnnotateStrokeCoordinate* cur_point = (AnnotateStrokeCoordinate*)listOut->data;
	  minx = MIN(minx,cur_point->x);
	  miny = MIN(miny,cur_point->y);
	  maxx = MAX(maxx,cur_point->x);
	  maxy = MAX(maxy,cur_point->y); 
	  g_free(listOut->data);
	  listOut = listOut->next;
	}   
      g_slist_free (savedListOut);
      listOut = NULL;

      AnnotateStrokeCoordinate* point0 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      point0->x = minx;
      point0->y = miny;
      point0->width = width;
      listOut = g_slist_prepend (listOut, point0);
                       
      AnnotateStrokeCoordinate* point1 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      point1->x = maxx;
      point1->y = miny;
      point1->width = width;
      listOut = g_slist_prepend (listOut, point1);

      AnnotateStrokeCoordinate* point2 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      point2->x = maxx;
      point2->y = maxy;
      point2->width = width;
      listOut = g_slist_prepend (listOut, point2);

      AnnotateStrokeCoordinate* point3 =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      point3->x = minx;
      point3->y = maxy;
      point3->width = width;
      listOut = g_slist_prepend (listOut, point3);

    }
  else
    {
      AnnotateStrokeCoordinate* last_point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
      last_point->x = Cx;
      last_point->y = Cy;
      last_point->width = width;
      listOut = g_slist_prepend (listOut, last_point);
    }


  return listOut;
}
