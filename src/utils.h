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

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


extern GtkBuilder *gtkBuilder;

#ifdef WIN32
  /* gdk_cursor_new_from_pixmap is broken on Windows.
     this is a workaround using gdk_cursor_new_from_pixbuf. */
  GdkCursor* fixed_gdk_cursor_new_from_pixmap(GdkPixmap *source, GdkPixmap *mask,
					    const GdkColor *fg, const GdkColor *bg,
					    gint x, gint y);
  #define gdk_cursor_new_from_pixmap fixed_gdk_cursor_new_from_pixmap
  #define LWA_COLORKEY	0x00000001
  #define LWA_ALPHA               0x00000002
#endif
						
/* get bar window widget */
GtkWidget* get_bar_window();


/* Ungrab pointer */
void ungrab_pointer(GdkDisplay* display, GtkWidget *win);


/* Grab pointer */
void grab_pointer(GtkWidget *win, GdkEventMask eventmask);


/* Take a GdkColor and return the RGB string */
gchar* gdkcolor_to_rgba(GdkColor* gdkcolor);


/* Set the cairo surface color to the RGBA string */
void cairo_set_source_color_from_string( cairo_t * cr, char* color);


/* Set the cairo surface color to transparent */
void  cairo_set_transparent_color(cairo_t * cr);


/** Distance beetween two points using the Pitagora theorem */
int get_distance(int x1, int y1, int x2, int y2);

/* Clear cairo context */
void clear_cairo_context(cairo_t* cr);

/* Transparent rectangle to allow to see the bar and pass the event below */
void make_hole(GtkWidget *widget, cairo_t * cr);


/*
 * This is function return if the point (x,y) in inside the ardesia bar window
 */
gboolean inside_bar_window(int xp, int yp);


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel
 */
GdkColor* rgb_to_gdkcolor(char* rgb);


/* Get the current date and format in a printable format */
gchar* get_date();


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir);


/*
 * Get the desktop folder;
 * Now this function use gconf to found the folder,
 * this means that this rutine works properly only
 * with the gnome desktop environment
 * We can investigate how-to do this
 * in a desktop environment independant way
 */
const gchar* get_desktop_dir (void);


