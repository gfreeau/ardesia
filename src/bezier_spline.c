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

#include <gtk/gtk.h>

#include <stdlib.h>
#include <annotate.h>
#include <math.h>
#include <gsl/gsl_linalg.h>


/* Spline the lines with a bezier curves */
void spline (cairo_t *cr, GSList *list)
{
  gint i;
  guint N = g_slist_length(list);
  double X[N][2]; 
  for  (i=0; i<N; i++)
    {
      AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, i); 
      X[i][0] = point->x;
      X[i][1] = point->y;
    }

  // Pi, Qi are control points for curve (Xi, Xi+1)
  double P[N-1][2], Q[N-1][2];

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
              x = inv(A)*b

       Pi, Qi and Xi are (x,y) pairs!
  
  *****************************************************************************/
  int s, eq = 0;
  gsl_matrix *m;
  gsl_vector *bx, *by, *x;
  gsl_permutation *perm;
   
  // allocate matrix and vectors
  m  = gsl_matrix_calloc(2*(N-1), 2*(N-1)); 
  bx = gsl_vector_calloc(2*(N-1)); 
  by = gsl_vector_calloc(2*(N-1)); 
  
  // fill-in matrix
  for ( i = 0; i < N-2; i++ ) 
    {
      gsl_matrix_set(m, eq, i+1, 1);        // Pi+1
      gsl_matrix_set(m, eq, (N-1)+i, 1);    // + Qi
      eq++;                                 // = 2Xi+1

      gsl_matrix_set(m, eq, i, 1);          // Pi
      gsl_matrix_set(m, eq, i+1, 2);        // + 2*Pi+1
      gsl_matrix_set(m, eq, (N-1)+i+1, -1); // - Qi+1
      gsl_matrix_set(m, eq, (N-1)+i, -2);   // - 2*Qi
      eq++;                                 // = 0
    }
  gsl_matrix_set(m, eq++, 0, 1);            // P0   = X0
  gsl_matrix_set(m, eq++, 2*(N-1)-1, 1);    // Qn-1 = Xn

  
  // fill-in vectors
  for ( i = 0; i < N-2; i++ ) 
    {
      gsl_vector_set(bx, 2*i, 2*X[i+1][0]);
      gsl_vector_set(by, 2*i, 2*X[i+1][1]);
    }
  gsl_vector_set(bx, 2*(N-1)-2, X[0][0]);
  gsl_vector_set(bx, 2*(N-1)-1, X[N-1][0]);

  gsl_vector_set(by, 2*(N-1)-2, X[0][1]);
  gsl_vector_set(by, 2*(N-1)-1, X[N-1][1]);
    
  // caluclate LU decomposition, solve lin. systems...  
  perm = gsl_permutation_alloc(2*(N-1));
  gsl_linalg_LU_decomp(m, perm, &s);

  // solve for bx
  x  = gsl_vector_calloc(2*(N-1));
  gsl_linalg_LU_solve( m, perm, bx, x ); 
  // copy solution (FIXME: should be avoided!)
  for ( i = 0; i < N-1; i++ )
    {
      P[i][0] = gsl_vector_get(x, i);
      Q[i][0] = gsl_vector_get(x, i+(N-1));
    }
  gsl_vector_free(x);

  // solve for by
  x  = gsl_vector_calloc(2*(N-1));
  gsl_linalg_LU_solve( m, perm, by, x ); 
  // copy solution (FIXME: should be avoided!)
  for ( i = 0; i < N-1; i++ )
    {
      P[i][1] = gsl_vector_get(x, i);
      Q[i][1] = gsl_vector_get(x, i+(N-1));
    }
  gsl_vector_free(x);


  gsl_permutation_free (perm);
  
  // free matrix and vectors
  gsl_matrix_free (m);
  gsl_vector_free (bx);
  gsl_vector_free (by);


  /* Now paint the smoothed line */
  
  for ( i = 0; i < N-1; i++ )
    {

      // B second derivates  
      //        printf("%d: Bx''(0) = %lf\n", i+1, 6*X[i][0]-12*P[i][0]+6*Q[i][0]);
      //        printf("%d: Bx''(1) = %lf\n\n", i+1, 6*P[i][0]-12*Q[i][0]+6*X[i+1][0]);

      // B first derivates
      //        printf("%d: Bx'(0) = %lf\n", i+1, -3*X[i][0]+3*P[i][0]);
      //        printf("%d: Bx'(1) = %lf\n", i+1, -3*Q[i][0]+3*X[i+1][0]);

      cairo_move_to(cr,
		    X[i][0], X[i][1]);

      cairo_curve_to(cr,
		     P[i][0], P[i][1],
		     Q[i][0], Q[i][1],
		     X[i+1][0], X[i+1][1]);                  
    }

}


