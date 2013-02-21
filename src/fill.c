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
                         
  gint colorint = (color[0] << 32) | (color[1] << 24) | (color[2] << 16) | (color[3] << 8);
  //printf("Color %8.8X \n", colorint);
  return colorint;
}


/* Is the color to coordinate (x,y) equals to the old pixel color. */
gboolean
is_old_pixel_value    (struct FillInfo *fill_info,
                       gdouble          x,
                       gdouble          y)
{
  gint c = get_color (fill_info, x, y);
  gint oc = fill_info->orig_color;
  if (c == oc)
     {
       return TRUE;
     }
     
  guint r, g, b, a, or, og, ob, oa;
  a = ((c & 0xff000000) >> 24);
  r = (c & 0x000000ff);
  g = ((c & 0x0000ff00) >> 8);
  b = ((c & 0x00ff0000) >> 16);
  
  oa = ((oc & 0xff000000) >> 24);
  or = (oc & 0x000000ff);
  og = ((oc & 0x0000ff00) >> 8);
  ob = ((oc & 0x00ff0000) >> 16);
 
  //sscanf (c, "%02X%02X%02X%02X", &r, &g, &b, &a);
  //sscanf (oc, "%02X%02X%02X%02X", &or, &og, &ob, &oa);
  
  gint threshold = 250;
  //gint difference = sqrt (pow (or-r, 2) + pow (og-g, 2) + pow (ob-b, 2) + pow (oa-a, 2) );
  
  gint difference = abs(or-r) + abs(og-g) + abs(ob-b) + abs(oa-a);
  //printf("Difference %d %d\n", difference, a);
  
  if ( difference < threshold)
    {
      return TRUE;
    }
  
  //printf("Color %8.8X \n", colorint);
  
  return FALSE; 
}


void set_new_pixel_value     (struct FillInfo   *fill_info,
                              gint               x,
                              gint               y)
{
  const guchar *s  = fill_info->pixels + y * fill_info->stride + x * 4;
  /* Put something defferent. */
  *(gint32 *)s = fill_info->orig_color -1;
}


/* Color a pixel. */
void
mark_pixel                   (cairo_t          *annotation_cairo_context,
                              gdouble           x,
                              gdouble           y)
{
  cairo_line_to (annotation_cairo_context, x, y);
}


/*
 * Internal flood fill function.
 * Algorithm based on SeedFill.c from GraphicsGems.
 */
static void flood_fill_int  (struct FillInfo *info,
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
          for (x = x1; (x >= 0) && is_old_pixel_value (info, x, y); x--)
            {
              set_new_pixel_value (info, x, y);
              mark_pixel (info->context, x, y); 
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
              for (; (x < info->width) && is_old_pixel_value (info, x, y); x++)
                {
                  set_new_pixel_value (info, x, y);
                  mark_pixel (info->context, x, y);
                }
              PUSH(y, l, x - 1, dy);
              if (x > x2 + 1)
                {
                  PUSH(y, x2 + 1, x - 1, -dy);
                }
skip:
              for (x++; x <= x2 && !is_old_pixel_value (info, x, y); x++)
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
  gint color = get_color (&fill_info, x, y);
  fill_info.orig_color = color;
  
  cairo_set_line_width (annotation_cairo_context, 1);
  
  flood_fill_int (&fill_info, x, y);

  cairo_stroke (annotation_cairo_context);
}


