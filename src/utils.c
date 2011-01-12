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

#ifdef _WIN32
#include <windows_utils.h>
#endif
	
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


/* Save the contents of the pixbuf in the file with name filename */
gboolean save_png (GdkPixbuf *pixbuf,const gchar *filename)
{
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *surface;
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(surface);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
  cairo_paint(cr);
  
  /* write in png */
  cairo_surface_write_to_png(surface, filename);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return TRUE;
}


/* Grab the screenshoot and put it in the GdkPixbuf */
GdkPixbuf* grab_screenshot()
{
  gint height = gdk_screen_height();
  gint width = gdk_screen_width();

  GdkWindow *root_window = gdk_get_default_root_window();
  return gdk_pixbuf_get_from_drawable (NULL, root_window, NULL,
                                       0, 0, 0, 0, width, height);
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


/* Drill the gdkwindow in the area where the ardesia bar is located */
void drill_window_in_bar_area(GdkWindow* window)
{
  /* Instantiate a trasparent pixmap with a black hole upon the bar area to be used as mask */
  GdkBitmap* shape = gdk_pixmap_new(NULL,  gdk_screen_width(), gdk_screen_height(), 1);
  cairo_t* shape_cr = gdk_cairo_create(shape);

  cairo_set_operator(shape_cr,CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (shape_cr, 1, 1, 1, 1);
  cairo_paint(shape_cr);

  GtkWidget* bar= get_bar_window();
  int x,y,width,height;
  gtk_window_get_position(GTK_WINDOW(bar),&x,&y);
  gtk_window_get_size(GTK_WINDOW(bar),&width,&height);

  cairo_set_operator(shape_cr,CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (shape_cr, 0, 0, 0, 0);
  cairo_rectangle(shape_cr, x, y, width, height);
  cairo_fill(shape_cr);	

  gdk_window_input_shape_combine_mask(window,
				      shape,
				      0, 0);
  cairo_destroy(shape_cr);
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
  gchar* date = g_strdup_printf("%d-%d-%d_%d%s%d-%d", t->tm_mday, t->tm_mon+1,t->tm_year+1900, t->tm_hour, hour_min_sep, t->tm_min, t->tm_sec);
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
gchar* get_default_name()
{
  gchar* date = get_date(); 
  gchar* filename = g_strdup_printf("%s_%s", PACKAGE_NAME, date);
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


/* Delete a directory recursively */
void rmdir_recursive (gchar *path)
{
  GDir *cur_dir;
  const gchar *dir_file;
			
  cur_dir = g_dir_open(path, 0, NULL);
  
  if (cur_dir)
    {
      while ((dir_file = g_dir_read_name(cur_dir)))
	{
	  gchar *fpath = g_build_filename(path, dir_file, NULL);
	
	  if (fpath) 
	    {
	      if (g_file_test(fpath, G_FILE_TEST_IS_DIR))
		{
		  rmdir_recursive(fpath);
		} 
	      else 
		{
		  g_unlink(fpath);
		}
	      g_free(fpath);
	    }
	}
			
      g_dir_close (cur_dir);
    }

  g_rmdir (path);
}


/* Allocate a new point belonging to the stroke passing the values */
AnnotateStrokeCoordinate * allocate_point(gint x, gint y, gint width, gdouble pressure)
{
  AnnotateStrokeCoordinate* point =  g_malloc (sizeof (AnnotateStrokeCoordinate));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  return point;
}


/* Send an email */
void send_email(gchar* to, gchar* subject, gchar* body, gchar* attachment)
{
#ifdef _WIN32
  windows_send_email(to, subject, body, attachment);
#else
  gchar* argv[9] = {"xdg-email", "--attach", attachment, "--subject", subject, "--body", body, to, (gchar*) 0};

  g_spawn_sync (NULL /*working_directory*/,
		argv,
		NULL /*envp*/,
		G_SPAWN_SEARCH_PATH,
		NULL /*child_setup*/,
		NULL /*user_data*/,
		NULL /*child_pid*/,
		NULL /*error*/,
		NULL,
		NULL);
#endif
}


/* Send trace with email */
void send_trace_with_email(gchar* attachment)
{
  gchar* to = "ardesia-developer@googlegroups.com";
  gchar* subject = "ardesia-bug-report";
  gchar* body = "Dear ardesia developer group,\nAn unhandled application error occurred, please for details see the attachment with the stack trace.";
  send_email(to, subject, body, attachment);
}


/* Is the desktop manager gnome */
gboolean is_gnome()
{
#ifdef _WIN32
  return FALSE;
#endif

  gchar* current_desktop = getenv("XDG_CURRENT_DESKTOP");
  if (current_desktop)
    {
      if (strcmp(current_desktop,"GNOME")!=0) {
	return FALSE;
      }
    }
  return TRUE;
}


