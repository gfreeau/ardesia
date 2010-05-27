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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "time.h"
#include <math.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <utils.h>
#include <gconf/gconf-client.h>


#ifdef _WIN32
  #include <cairo-win32.h>
  #include <winuser.h>  
#else
  #ifdef __APPLE__
    #include <cairo-quartz.h>
  #else
    #include <cairo-xlib.h>
  #endif
#endif


#ifdef _WIN32
  #include <windows.h>
  #define DIR_SEPARATOR '\\'
#else
  #define DIR_SEPARATOR '/'
#endif


const gchar* desktop_dir = NULL;


#ifdef WIN32

/* Is the two color similar */
gboolean colors_too_similar(const GdkColor *colora, const GdkColor *colorb)
{
  return (abs(colora->red - colorb->red) < 256 &&
	  abs(colora->green - colorb->green) < 256 &&
	  abs(colora->blue - colorb->blue) < 256);
}


/* 
 * gdk_cursor_new_from_pixmap is broken on Windows.
 * this is a workaround using gdk_cursor_new_from_pixbuf. 
 *  Thanks to Dirk Gerrits for this contribution 
 */
GdkCursor* fixed_gdk_cursor_new_from_pixmap(GdkPixmap *source, GdkPixmap *mask,
					    const GdkColor *fg, const GdkColor *bg,
					    gint x, gint y)
{
  GdkPixmap *rgb_pixmap;
  GdkGC *gc;
  GdkPixbuf *rgb_pixbuf, *rgba_pixbuf;
  GdkCursor *cursor;
  int width, height;

  /* HACK!  It seems impossible to work with RGBA pixmaps directly in
     GDK-Win32.  Instead we pick some third color, different from fg
     and bg, and use that as the '<b style="color: black; background-color: rgb(255, 153, 153);">transparent</b> color'.  We do this using
     colors_too_similar (see above) because two colors could be
     unequal in GdkColor's 16-bit/sample, but equal in GdkPixbuf's
     8-bit/sample. */
  GdkColor candidates[3] = {{0,65535,0,0}, {0,0,65535,0}, {0,0,0,65535}};
  GdkColor *trans = &candidates[0];
  if (colors_too_similar(trans, fg) || colors_too_similar(trans, bg)) {
    trans = &candidates[1];
    if (colors_too_similar(trans, fg) || colors_too_similar(trans, bg)) {
      trans = &candidates[2];
    }
  } /* trans is now guaranteed to be unique from fg and bg */

  /* create an empty pixmap to hold the cursor image */
  gdk_drawable_get_size(source, &width, &height);
  rgb_pixmap = gdk_pixmap_new(NULL, width, height, 24);

  /* blit the bitmaps defining the cursor onto a <b style="color: black; background-color: rgb(255, 153, 153);">transparent</b> background */
  gc = gdk_gc_new(rgb_pixmap);
  gdk_gc_set_fill(gc, GDK_SOLID);
  gdk_gc_set_rgb_fg_color(gc, trans);
  gdk_draw_rectangle(rgb_pixmap, gc, TRUE, 0, 0, width, height);
  gdk_gc_set_fill(gc, GDK_OPAQUE_STIPPLED);
  gdk_gc_set_stipple(gc, source);
  gdk_gc_set_clip_mask(gc, mask);
  gdk_gc_set_rgb_fg_color(gc, fg);
  gdk_gc_set_rgb_bg_color(gc, bg);
  gdk_draw_rectangle(rgb_pixmap, gc, TRUE, 0, 0, width, height);
  gdk_gc_unref(gc);

  /* create a cursor out of the created pixmap */
  rgb_pixbuf = gdk_pixbuf_get_from_drawable(
                                            NULL, 
                                            rgb_pixmap, 
                                            gdk_colormap_get_system(),
                                            0, 0, 0, 0,
                                            width, 
                                            height);
  gdk_pixmap_unref(rgb_pixmap);
  rgba_pixbuf = gdk_pixbuf_add_alpha(rgb_pixbuf, TRUE, trans->red, trans->green, trans->blue);
  gdk_pixbuf_unref(rgb_pixbuf);
  cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), rgba_pixbuf, x, y);
  gdk_pixbuf_unref(rgba_pixbuf);

  return cursor;
}
#endif


/* get bar window widget */
GtkWidget* get_bar_window()
{
  return GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"winMain"));
}


/** Distance beetween two points using the Pitagora theorem */
int get_distance(int x1, int y1, int x2, int y2)
{
  if ((x1==x2) && (y1==y2))
    {
      return 0;
    }
  int ret = (sqrt(pow(x1-x2,2) + pow(y1-y2,2)));
  return ret; 
}


/* Take a GdkColor and return the RGBA string */
gchar* gdkcolor_to_rgba(GdkColor* gdkcolor)
{
  gchar* ret= g_malloc(9 * sizeof(gchar));;
  /* transform in the  RGBA format e.g. FF0000FF */ 
  sprintf(ret,"%02x%02x%02xFF", gdkcolor->red/257, gdkcolor->green/257, gdkcolor->blue/257);
  return ret;
}


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel
 */
GdkColor* rgb_to_gdkcolor(char* rgb)
{
  GdkColor* gdkcolor = g_malloc(sizeof(GdkColor));
  gchar *ccolor = g_malloc(8 * sizeof (gchar));
  ccolor[0]='#';
  strncpy(&ccolor[1], rgb, 6);
  ccolor[7]=0;
  gdk_color_parse (ccolor, gdkcolor);
  g_free(ccolor);
  return gdkcolor;
}


/* Clear cairo context */
void clear_cairo_context(cairo_t* cr)
{
  if (cr)
    {
      cairo_save(cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr);
      cairo_restore(cr);
    } 
}


/* Set the cairo surface color to the RGBA string */
void cairo_set_source_color_from_string( cairo_t * cr, char* color)
{
 if (cr)
   {
     int r,g,b,a;
     sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);
     cairo_set_source_rgba (cr, (double) r/256, (double) g/256, (double) b/256, (double) a/256);
   }
}


/*
 * This is function return if the point (x,y) in inside the ardesia bar window
 */
gboolean inside_bar_window(int xp, int yp)
{
  int x, y, width, height;
  GtkWindow* bar = GTK_WINDOW(get_bar_window());
  gtk_window_get_position(bar, &x, &y);
  gtk_window_get_size(bar, &width, &height);   
  
  /* rectangle that contains the panel */
  if ((yp>=y)&&(yp<y+height))
    {
      if ((xp>=x)&&(xp<x+width))
	{
	  return 1;
	}
    }
  return 0;
}


/* Get the current date and format in a printable format */
gchar* get_date()
{
  struct tm* t;
  time_t now;
  time( &now );
  t = localtime( &now );

  gchar* date = g_malloc(19 * sizeof(gchar));
  sprintf(date, "%d-%d-%d_%d:%d:%d", t->tm_mday,t->tm_mon+1,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec);
  return date;
}


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir)
{
  char* afterslash = strrchr(filename, DIR_SEPARATOR);

  if (afterslash == 0)
    {
      /* relative path */
      filename = strcat(filename,desktop_dir);
    }
  struct stat statbuf;
  if(stat(filename, &statbuf) < 0) {
    if(errno == ENOENT) {
      return FALSE;
    } else {
      exit(0);
    }
  }
  return TRUE;
}


/*
 * Get the desktop folder;
 * Now this function use gconf to found the folder,
 * this means that this rutine works properly only
 * with the gnome desktop environment
 * We can investigate how-to do this
 * in a desktop environment independant way
 */
const gchar* get_desktop_dir (void)
{
  const gchar* desktop_dir = NULL;
  GConfClient *gconf_client = NULL;
  gboolean desktop_is_home_dir = FALSE;

  gconf_client = gconf_client_get_default ();

  /* in windows gconf daemon is surely not running */
  #ifndef _WIN32
    desktop_is_home_dir = gconf_client_get_bool (gconf_client,
                                                 "/apps/nautilus/preferences/desktop_is_home_dir",
                                                 NULL);
  #endif

  if (desktop_is_home_dir)
    {
      desktop_dir = g_get_home_dir ();
    }
  else
    {
      desktop_dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
    }

  g_object_unref (gconf_client);

  return desktop_dir;
}

