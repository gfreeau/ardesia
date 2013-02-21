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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#  include <cairo-win32.h>
#else
#  ifdef __APPLE__
#    include <cairo-quartz.h>
#  else
#    include <cairo-xlib.h>
#  endif
#endif


/* Stack size used for flood fill algorithm. */
#define STACKSIZE 10000


/* Macro to get the rgba value of the pixel at coordinate (x,y).*/ 
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAIRO_ARGB32_GET_PIXEL(s, r, g, b, a)      \
  G_STMT_START {                                   \
    const guint tb = (s)[0];                       \
    const guint tg = (s)[1];                       \
    const guint tr = (s)[2];                       \
    const guint ta = (s)[3];                       \
    (r) = (tr << 8) / (ta + 1);                    \
    (g) = (tg << 8) / (ta + 1);                    \
    (b) = (tb << 8) / (ta + 1);                    \
    (a) = ta;                                      \
  } G_STMT_END
#else
#define CAIRO_ARGB32_GET_PIXEL(s, r, g, b, a)      \
  G_STMT_START {                                   \
    const guint ta = (s)[0];                       \
    const guint tr = (s)[1];                       \
    const guint tg = (s)[2];                       \
    const guint tb = (s)[3];                       \
    (r) = (tr << 8) / (ta + 1);                    \
    (g) = (tg << 8) / (ta + 1);                    \
    (b) = (tb << 8) / (ta + 1);                    \
    (a) = ta;                                      \
  } G_STMT_END
#endif


/* Struct used to store the point visited by the flood fill algorithm. */
struct FillPixelInfo
{
   int y, xl, xr, dy;
};


/* Push macro used by flood fill algorithm. */
#define PUSH(py, pxl, pxr, pdy)                                     \
{                                                                   \
    struct FillPixelInfo *p = sp;                                   \
    if ( ( (py) + (pdy) >= 0) && ( (py) + (pdy) < info->height))    \
    {                                                               \
        p->y = (py);                                                \
        p->xl = (pxl);                                              \
        p->xr = (pxr);                                              \
        p->dy = (pdy);                                              \
        sp++;                                                       \
    }                                                               \
}


/* Pop macro used by flood fill algorithm. */
#define POP(py, pxl, pxr, pdy)                                      \
{                                                                   \
    sp--;                                                           \
    (py) = sp->y + sp->dy;                                          \
    (pxl) = sp->xl;                                                 \
    (pxr) = sp->xr;                                                 \
    (pdy) = sp->dy;                                                 \
}


/* Struct passed to flood fill. */
struct FillInfo
{

  /* Cairo surface used to visit the image. */
  cairo_surface_t *surface;
  
  /* Surface width. */
  gint width;
  
  /* Surface height. */
  gint height;

  /* Cairo context used to mark pixels. */
  cairo_t *context;

  /* Color at initial point. */
  gint orig_color;

  /* Pixels of image. */
  guchar *pixels;

  /* Stride of image. */
  gint stride;

};


/*
 * Get color of the surface at point with coordinates (x,y).
 */
gint
get_color             (cairo_surface_t  *surface,
                       gint              x,
                       gint              y);


/*
 * Perform the flood fill algorithm.
 */
void
flood_fill                   (cairo_t          *annotation_cairo_context,
                              cairo_surface_t  *surface,
                              gdouble           x,
                              gdouble           y);


