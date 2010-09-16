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

#include <utils.h>


	
/* Grab pointer */
void grab_pointer(GtkWidget *win, GdkEventMask eventmask)
{
  GdkGrabStatus result;
  gdk_error_trap_push();    
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
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      /* this probably means the device table is outdated, 
	 e.g. this device doesn't exist anymore 
       */
      g_printerr("Ungrab pointer device error\n");
    }
}


/* get bar window widget */
GtkWidget* get_bar_window()
{
  return GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"winMain"));
}


/** Get the distance beetween two points */
gdouble get_distance(gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
  if ((x1==x2) && (y1==y2))
    {
      return 0;
    }
  /* apply the Pitagora theorem to calculate the distance */
  return (sqrtf(powf(x1-x2,2) + powf(y1-y2,2)));
}


/* Take a GdkColor and return the corrispondent RGBA string */
gchar* gdkcolor_to_rgb(GdkColor* gdkcolor)
{
  /* transform in the  RGB format e.g. FF0000 */ 
  return g_strdup_printf("%02x%02x%02x", gdkcolor->red/257, gdkcolor->green/257, gdkcolor->blue/257);
}


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel; the gtkColor doesn't support the rgba color
 */
GdkColor* rgba_to_gdkcolor(gchar* rgba)
{
  GdkColor* gdkcolor = g_malloc(sizeof(GdkColor));
  gchar *rgb = g_strndup(rgba, 6);
  gchar *color = g_strdup_printf("%s%s", "#", rgb);
  gdk_color_parse (color, gdkcolor);
  g_free(color);
  g_free(rgb);
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
void cairo_set_source_color_from_string( cairo_t * cr, gchar* color)
{
 if (cr)
   {
     gint r,g,b,a;
     sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);
     cairo_set_source_rgba (cr, (gdouble) r/255, (gdouble) g/255, (gdouble) b/255, (gdouble) a/255);
   }
}


/*
 * This is function return if the point (x,y) in inside the ardesia bar window
 */
gboolean inside_bar_window(gdouble xp, gdouble yp)
{
  gint x, y, width, height;
  GtkWindow* bar = GTK_WINDOW(get_bar_window());
  gtk_window_get_position(bar, &x, &y);
  gtk_window_get_size(bar, &width, &height);   
  
  if ((yp>=y)&&(yp<y+height))
    {
      if ((xp>=x)&&(xp<x+width))
	{
	  return 1;
	}
    }
  return 0;
}


/* 
 * Get the current date and format in a printable format; 
 * the returned value must be free with the g_free 
 */
gchar* get_date()
{
  struct tm* t;
  time_t now;
  time( &now );
  t = localtime( &now );

  gchar* hour_min_sep = ":";
  #ifdef _WIN32
    /* The ":" character on windows is avoided in file name and then I use the "." character instead */
    hour_min_sep = ".";
  #endif
  gchar* date = g_strdup_printf("%d-%d-%d_%d%s%d", t->tm_mday, t->tm_mon+1,t->tm_year+1900, t->tm_hour, hour_min_sep, t->tm_min);
  return date;
}


/* Return if a file exists */
gboolean file_exists(gchar* filename)
{
  return g_file_test(filename, G_FILE_TEST_EXISTS);
}


/* 
 * Get default name return a name containing the tool name and the current date; 
 * the returned value must be free with the g_free 
 */
gchar * get_default_name()
{
  gchar* start_string = "ardesia_";
  gchar* date = get_date(); 
  gchar* filename = g_strdup_printf("%s%s", start_string, date);
  g_free(date); 
  return filename;
}


/*
 * Get the desktop folder
 */
const gchar* get_desktop_dir (void)
{
  return g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
}


