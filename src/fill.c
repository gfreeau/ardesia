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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fill.h>
#include <utils.h>

/*
 * Get color of the surface at point with coordinates (x,y).
 */
static gint
get_color             (struct FillInfo *fill_info,
                       gint           x,
                       gint           y)
{
  guchar *pixels = fill_info->pixels;
  gint stride = fill_info->stride;
  const guchar *s  = pixels + y * stride + x * 4;

  guchar color[4];
  CAIRO_ARGB32_GET_PIXEL(s,
                         color[0],
                         color[1],
                         color[2],
                         color[3]);

  return RGBA_TO_UINT(color[0], color[1], color[2], color[3]);
}


/* Is the color to coordinate (x, y) equals to the old pixel color. */
static gboolean
is_similar_to_old_pixel_value    (struct FillInfo *fill_info,
                                  gdouble          x,
                                  gdouble          y)
{
  gint c = get_color (fill_info, x, y);
  gint oc = fill_info->orig_color;
  if (c == oc)
    {
      return TRUE;
    }
  else
    {
      gfloat threshold = 255;
      guint16 r, g, b, a, or, og, ob, oa;
      r = UINT_RGBA_R(c);
      g = UINT_RGBA_G(c);
      b = UINT_RGBA_B(c);
      a = UINT_RGBA_A(c);

      or = UINT_RGBA_R(oc);
      og = UINT_RGBA_G(oc);
      ob = UINT_RGBA_B(oc);
      oa = UINT_RGBA_A(oc);

      //printf("Color %d %d %d %d , %d %d %d %d\n", r, g, b, a, or, og, ob, oa);
      //printf ("delta A %d\n", abs(oa-a));
      
      guint deltaa = abs (oa-a);
      gfloat deltar = fabs (or*oa/256.0-r*a/256.0);
      gfloat deltag = fabs (og*oa/256.0-g*a/256.0);
      gfloat deltab = fabs (ob*oa/256.0-b*a/256.0);

      /* 
       * It excludes the points with alpha value used for transparent, semitranparent and opaque color.
       * This is a rough way to detect the borders.
       * The points are filtered on threshold and balanced with alpha channel.
       */
      if ( (deltaa != 255)                   &&
         ( (deltaa < 135) || (deltaa > 136)) &&
         (deltaa != 0)                       &&
         (deltar < threshold)                &&
         (deltag < threshold)                &&
         (deltab < threshold))
        {
          return TRUE;
        }
    }

  return FALSE; 
}


/* Set the new pixel value. */
void set_new_pixel_value     (struct FillInfo   *fill_info,
                              gint               x,
                              gint               y)
{
  guchar *s  = fill_info->pixels + y * fill_info->stride + x * 4;
  guint16 r, g, b, a;
  r = UINT_RGBA_R(fill_info->filled_color);
  g = UINT_RGBA_G(fill_info->filled_color);
  b = UINT_RGBA_B(fill_info->filled_color);
  a = UINT_RGBA_A(fill_info->filled_color);

  /* Set the new pixel value. */
  CAIRO_ARGB32_SET_PIXEL(s, r, g, b, a);
}


/*
 * Internal flood fill function.
 * Algorithm based on SeedFill.c from GraphicsGems.
 */
static void flood_fill_internal  (struct FillInfo *info,
                                  gdouble          x,
                                  gdouble          y)
{
  /* TODO: check for stack overflow? */
  /* TODO: that's a lot of memory! esp if we never use it */

  struct FillPixelInfo stack[STACKSIZE];
  struct FillPixelInfo * sp = stack;
  int l, x1, x2, dy;

  if ( (x >= 0) && (x < info->width) && (y >= 0) && (y < info->height))
    {
      PUSH(y, x, x, 1);
      PUSH(y + 1, x, x, -1);
      while (sp > stack)
        {
          POP(y, x1, x2, dy);
          for (x = x1; (x >= 0) && is_similar_to_old_pixel_value (info, x, y); x--)
            {
              set_new_pixel_value (info, x, y);
            }
          if (x >= x1)
            {
              goto skip;
            }
          l = x + 1;
          if (l < x1)
            {
              PUSH(y, l, x1 - 1, -dy);
            }
          x = x1 + 1;
          do
            {
              for (; (x < info->width) && is_similar_to_old_pixel_value (info, x, y); x++)
                {
                  set_new_pixel_value (info, x, y);
                }
              PUSH(y, l, x - 1, dy);
              if (x > x2 + 1)
                {
                  PUSH(y, x2 + 1, x - 1, -dy);
                }
skip:
              for (x++; x <= x2 && !is_similar_to_old_pixel_value (info, x, y); x++)
                {
                  // empty block;
                }
              l = x;
            } while (x <= x2);
        }
    }
}


/* It perform the flood fill algorithm starting from point with coordinate (x,y). */
void
flood_fill                   (cairo_t          *annotation_cairo_context,
                              cairo_surface_t  *surface,
                              gchar            *filled_color,
                              gdouble           x,
                              gdouble           y)
{

  struct FillInfo fill_info;
  
  fill_info.width = cairo_image_surface_get_width (surface);
  fill_info.height = cairo_image_surface_get_height (surface);
  fill_info.surface = surface;
  fill_info.context = annotation_cairo_context;
  fill_info.pixels = cairo_image_surface_get_data (surface);
  fill_info.stride = cairo_image_surface_get_stride (surface);
  fill_info.orig_color = get_color (&fill_info, x, y);

  guint r, g, b, a;
  sscanf (filled_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
  gint filled_colorint = RGBA_TO_UINT(r, g, b, a);

  if (filled_colorint == fill_info.orig_color)
    {
      return;
    }

  fill_info.filled_color = filled_colorint;

  flood_fill_internal (&fill_info, x, y);

  cairo_set_source_surface (annotation_cairo_context, surface, 0, 0);
  cairo_paint (annotation_cairo_context);
  cairo_stroke (annotation_cairo_context);
  
}


