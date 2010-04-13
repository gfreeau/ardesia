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


#if defined(_WIN32)
  #include <cairo-win32.h>
  #include <gdkwin32.h>
#else
  #include <cairo.h>
  #include <cairo-xlib.h>
  #include <gconf/gconf-client.h>
#endif


const gchar* desktop_dir = NULL;

gboolean colors_too_similar(const GdkColor *colora, const GdkColor *colorb)
{
  return (abs(colora->red - colorb->red) < 256 &&
	  abs(colora->green - colorb->green) < 256 &&
	  abs(colora->blue - colorb->blue) < 256);
}

/* gdk_cursor_new_from_pixmap is broken on Windows.
   this is a workaround using gdk_cursor_new_from_pixbuf. */
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
     and bg and use that as the 'transparent color'.  We do this using
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
  } 
 /* trans is now guaranteed to be unique from fg and bg */
 trans = &candidates[2]; 

  /* create an empty pixmap to hold the cursor image */
  gdk_drawable_get_size(source, &width, &height);
  rgb_pixmap = gdk_pixmap_new(NULL, width, height, 24);

  /* blit the bitmaps defining the cursor onto a transparent background */
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
  rgb_pixbuf = gdk_pixbuf_get_from_drawable(NULL, rgb_pixmap, gdk_colormap_get_system(),
					    0, 0, 0, 0, width, height);
  gdk_pixmap_unref(rgb_pixmap);
  rgba_pixbuf = gdk_pixbuf_add_alpha(rgb_pixbuf, TRUE, trans->red, trans->green, trans->blue);
  gdk_pixbuf_unref(rgb_pixbuf);
  cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), rgba_pixbuf, x, y);
  gdk_pixbuf_unref(rgba_pixbuf);

  return cursor;
}


/* get bar window widget */
GtkWidget* get_bar_window()
{
  return GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"winMain"));
}

/* Grab pointer */
void grab_pointer(GtkWidget *win, GdkEventMask eventmask)
{
  gtk_widget_show (win);
  GdkGrabStatus result;
  gdk_error_trap_push(); 
  gtk_widget_input_shape_combine_mask(win, NULL, 0, 0);   
  result = gdk_pointer_grab (win->window, FALSE,
			     eventmask, 0,
			     NULL,
			     GDK_CURRENT_TIME); 

  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_printerr("Grab pointer error\n");
    }
   
  switch (result)
    {
    case GDK_GRAB_SUCCESS:
      break;
    case GDK_GRAB_ALREADY_GRABBED:
      g_printerr ("Grab Pointer failed: AlreadyGrabbed\n");
      break;
    case GDK_GRAB_INVALID_TIME:
      g_printerr ("Grab Pointer failed: GrabInvalidTime\n");
      break;
    case GDK_GRAB_NOT_VIEWABLE:
      g_printerr ("Grab Pointer failed: GrabNotViewable\n");
      break;
    case GDK_GRAB_FROZEN:
      g_printerr ("Grab Pointer failed: GrabFrozen\n");
      break;
    default:
      g_printerr ("Grab Pointer failed: Unknown error\n");
    }       

}

/* Ungrab pointer */
void ungrab_pointer(GdkDisplay* display, GtkWidget* win)
{
  gdk_error_trap_push ();
  gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
  /* inherit cursor from root window */
  gdk_window_set_cursor (win->window, NULL);
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      /* this probably means the device table is outdated, 
	 e.g. this device doesn't exist anymore */
      g_printerr("Ungrab pointer device error\n");
    }
}


/* Take a GdkColor and return the RGBA string */
char* gdkcolor_to_rgba(GdkColor* gdkcolor)
{
  char* ret= g_malloc(9*sizeof(char));;
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
  GdkColor* gdkcolor = g_malloc (sizeof (GdkColor));
  gchar *ccolor = g_malloc(8);
  ccolor[0]='#';
  strncpy(&ccolor[1], rgb, 6);
  ccolor[7]=0;
  gdk_color_parse (ccolor, gdkcolor);
  g_free(ccolor);
  return gdkcolor;
}


/* Set the cairo surface color to transparent */
void  cairo_set_transparent_color(cairo_t * cr)
{
  if (cr)
    {
      cairo_set_source_rgba (cr, 1, 1, 1, 0);
    }
}


/* Clear cairo context */
void clear_cairo_context(cairo_t* cr)
{
  if (cr)
    {
      cairo_save(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
      cairo_set_transparent_color(cr);
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


/* Get the current date and format in a printable format */
char* get_date()
{
  struct tm* t;
  time_t now;
  time( &now );
  t = localtime( &now );

  char* date = g_malloc(64*sizeof(char));
  sprintf(date, "%d-%d-%d_%d:%d:%d", t->tm_mday,t->tm_mon+1,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec);
  return date;
}


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir)
{
  char* afterslash = NULL; 
  #ifdef _WIN32
    //TODO: check if this works on win32
    afterslash = strrchr(filename, '\\');
  #else
    afterslash = strrchr(filename, '/');
  #endif
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
  #ifdef _WIN32
    //TODO: now we return an empty string; it is possbile that the file will be stored in the desktop
    desktop_dir = "";
  #else
    GConfClient *gconf_client = NULL;
    gboolean desktop_is_home_dir = FALSE;

    gconf_client = gconf_client_get_default ();
    desktop_is_home_dir = gconf_client_get_bool (gconf_client,
                                               "/apps/nautilus/preferences/desktop_is_home_dir",
                                               NULL);
    if (desktop_is_home_dir)
      {
        desktop_dir = g_get_home_dir ();
      }
    else
      {
        desktop_dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
      }

    g_object_unref (gconf_client);
  #endif

  return desktop_dir;
}

