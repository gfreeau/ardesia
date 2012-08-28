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


#include <bezier_spline.h>
#include <annotation_window.h>
#include <utils.h>


/* Spline the lines with a bezier curves. */
GSList *
spline (GSList *list)
{
  GSList *ret = NULL;
  guint i = 0;
  guint lenght = g_slist_length (list);
  gdouble mx[lenght][2]; 
  gdouble width = 12;
  gdouble pressure = 1;

  /* Pi, Qi are control points for curve (Xi, Xi+1). */
  gdouble mp[lenght-1][2];
  gdouble mq[lenght-1][2];

  gint s, eq = 0;
  gsl_matrix *m;
  gsl_vector *bx, *by, *x;
  gsl_permutation *perm;

  for  (i=0; i<lenght; i++)
    {
      AnnotatePoint *point = (AnnotatePoint*) g_slist_nth_data (list, i);
      mx[i][0] = point->x;
      mx[i][1] = point->y;
      if (i==0)
        {
          width = point->width;
          pressure = point->pressure;
        }
    }

  /*****************************************************************************
  
   Bezier control points system matrix
 1 
    P0 P1 P2 Pn-1 ... Q0 Q1 Q2 Qn-1       
   /    1              1           \   /P0  \      /  2*X1\ Pi+1 + Qi = 2*Xi+1
   | 1  2             -2 -1        |   |P1  |      |     0|
   |       1              1        |   |P2  |      |  2*X2|
   |    1  2             -2 -1     | * |Pn-1|   =  |     0| Pi + 2*Pi+1
   |          1              1     |   |Q0  |      |2*Xn-1|    - Qi+1 - 2*Qi = 0
   |       1  2             -2 -1  |   |Q1  |      |     0|
   | 1                             |   |Q2  |      |    X0| P0   = X0     
   \                             1 /   \Qn-1/      \    Xn/ Qn-1 = Xn

                A*x = b
              x = inv (A)*b

       Pi, Qi and Xi are (x,y) pairs!

  *****************************************************************************/

  /* Allocate matrix and vectors. */
  m  = gsl_matrix_calloc (2* (lenght-1), 2* (lenght-1)); 
  bx = gsl_vector_calloc (2* (lenght-1)); 
  by = gsl_vector_calloc (2* (lenght-1)); 

  /* Fill-in matrix. */
  for ( i = 0; i < lenght-2; i++ ) 
    {
      gsl_matrix_set (m, eq, i+1, 1);        // Pi+1
      gsl_matrix_set (m, eq, (lenght-1)+i, 1);    // + Qi
      eq++;                                 // = 2Xi+1

      gsl_matrix_set (m, eq, i, 1);          // Pi
      gsl_matrix_set (m, eq, i+1, 2);        // + 2*Pi+1
      gsl_matrix_set (m, eq, (lenght-1)+i+1, -1); // - Qi+1
      gsl_matrix_set (m, eq, (lenght-1)+i, -2);   // - 2*Qi
      eq++;                                 // = 0
    }

  gsl_matrix_set (m, eq++, 0, 1);            // P0   = X0
  gsl_matrix_set (m, eq++, 2* (lenght-1)-1, 1);    // Qn-1 = Xn

  
  /* Fill-in vectors. */
  for ( i = 0; i < lenght-2; i++ ) 
    {
      gsl_vector_set (bx, 2*i, 2*mx[i+1][0]);
      gsl_vector_set (by, 2*i, 2*mx[i+1][1]);
    }
  gsl_vector_set (bx, 2* (lenght-1)-2, mx[0][0]);
  gsl_vector_set (bx, 2* (lenght-1)-1, mx[lenght-1][0]);

  gsl_vector_set (by, 2* (lenght-1)-2, mx[0][1]);
  gsl_vector_set (by, 2* (lenght-1)-1, mx[lenght-1][1]);

  /* Calculate LU decomposition, solve lin. systems... */
  perm = gsl_permutation_alloc (2* (lenght-1));
  gsl_linalg_LU_decomp (m, perm, &s);

  /* Solve for bx. */
  x  = gsl_vector_calloc (2* (lenght-1));
  gsl_linalg_LU_solve ( m, perm, bx, x ); 
  /* copy solution (@FIXME: should be avoided!) */
  for ( i = 0; i < lenght-1; i++ )
    {
      mp[i][0] = gsl_vector_get (x, i);
      mq[i][0] = gsl_vector_get (x, i+ (lenght-1));
    }
  gsl_vector_free (x);

  /* Solve for by. */
  x  = gsl_vector_calloc (2* (lenght-1));
  gsl_linalg_LU_solve ( m, perm, by, x ); 
  /* copy solution (@FIXME: should be avoided!) */
  for ( i = 0; i < lenght-1; i++ )
    {
      mp[i][1] = gsl_vector_get (x, i);
      mq[i][1] = gsl_vector_get (x, i+ (lenght-1));
    }

  gsl_vector_free (x);

  gsl_permutation_free (perm);
  
  /* Free matrix and vectors. */
  gsl_matrix_free (m);
  gsl_vector_free (bx);
  gsl_vector_free (by);

  /* Now paint the smoothed line. */
  for ( i = 0; i < lenght-1; i++ )
    {

      /* B second derivates */  
      // printf ("%d: Bx'' (0) = %lf\n", i+1, 6*mx[i][0]-12*mp[i][0]+6*mq[i][0]);
      // printf ("%d: Bx'' (1) = %lf\n\n", i+1, 6*mp[i][0]-12*mq[i][0]+6*mx[i+1][0]);

      /* B first derivates */
      // printf ("%d: Bx' (0) = %lf\n", i+1, -3*mx[i][0]+3*mp[i][0]);
      // printf ("%d: Bx' (1) = %lf\n", i+1, -3*mq[i][0]+3*mx[i+1][0]);

      AnnotatePoint *first_point =  allocate_point ( mp[i][0],
                                                     mp[i][1],
                                                     width,
                                                     pressure);

      AnnotatePoint *second_point =  allocate_point (mq[i][0],
                                                     mq[i][1],
                                                     width,
                                                     pressure);

      AnnotatePoint *third_point =  allocate_point (mx[i+1][0],
                                                    mx[i+1][1],
                                                    width,
                                                    pressure);

      ret = g_slist_prepend (ret, first_point);
      ret = g_slist_prepend (ret, second_point);
      ret = g_slist_prepend (ret, third_point);

    }

  ret = g_slist_reverse (ret);
  return ret;
}


