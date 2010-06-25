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


const gchar* desktop_dir = NULL;


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

  gchar* date = g_malloc(16 * sizeof(gchar));
  char* hour_min_sep = ":";
  #ifdef _WIN32
    /* : is avoided in file name and the I use the . */
	hour_min_sep = ".";
  #endif
  
  sprintf(date, "%d-%d-%d_%d%s%d", t->tm_mday, t->tm_mon+1,t->tm_year+1900, t->tm_hour, hour_min_sep, t->tm_min);
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

#ifndef _WIN32
const gchar* glib_get_desktop_dir()
{
  return g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
}
#endif


/*
 * Get the desktop folder
 */
const gchar* get_desktop_dir (void)
{
  #ifndef _WIN32
    desktop_dir = glib_get_desktop_dir();
  #else
    // WIN32
    desktop_dir = win_get_desktop_dir();   
  #endif
  return desktop_dir;
}


