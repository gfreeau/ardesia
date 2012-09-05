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

#include <utils.h>

#ifdef _WIN32
#  include <windows_utils.h>
#endif

/* The name of the current project. */
static gchar *project_name = (gchar *) NULL;

/* The name of the current project. */
static gchar *project_dir = (gchar *) NULL;

/* The name of the current project. */
static gchar *iwb_filename = (gchar *) NULL;

/* The list of the artefacts created in the current session. */
static GSList *artifacts = (GSList *) NULL;


/* Get the name of the current project. */
gchar*
get_project_name   ()
{
  return project_name;
}


/* Set the name of the current project. */
void
set_project_name   (gchar  *name)
{
  project_name = name;
}


/* Get the dir of the current project. */
gchar*
get_project_dir    ()
{
  return project_dir;
}


/* Set the directory of the current project. */
void
set_project_dir    (gchar *dir)
{
  project_dir = dir;
}


/* Get the iwb file of the current project. */
gchar *
get_iwb_filename   ()
{
  return iwb_filename;
}


/* Set the iwb file of the current project. */
void
set_iwb_filename   (gchar *file)
{
  iwb_filename = file;
}


/* Get the list of the path of the artefacts created in the session. */
GSList *
get_artifacts      ()
{
  return artifacts;
}


/* Add the path of an artefacts created in the session to the list. */
void
add_artifact       (gchar *path)
{
  gchar *copied_path = g_strdup_printf ("%s", path);
  artifacts = g_slist_prepend (artifacts, copied_path);
}


/* Free the structure containing the artefact list created in the session. */
void
free_artifacts     ()
{
  g_slist_foreach (artifacts, (GFunc)g_free, NULL);
}


/* Get the bar window widget. */
GtkWidget *
get_bar_window     ()
{
  return GTK_WIDGET (gtk_builder_get_object (bar_gtk_builder, "winMain"));
}


/** Get the distance between two points. */
gdouble
get_distance       (gdouble x1,
                    gdouble y1,
                    gdouble x2,
                    gdouble y2)
{
  /* Apply the Pitagora theorem to calculate the distance. */
  gdouble x_delta = fabs(x2-x1);
  gdouble y_delta = fabs(y2-y1);
  gdouble quad_sum = pow (x_delta, 2);
  quad_sum = quad_sum + pow (y_delta, 2);
  return sqrt (quad_sum);
}


/* Take a GdkColor and return the equivalent RGBA string. */
gchar *
gdkcolor_to_rgb    (GdkColor *gdkcolor)
{
  /* Transform in the  RGB format e.g. FF0000. */ 
  gchar *ret_str = g_strdup_printf ("%02x%02x%02x",
                                    gdkcolor->red/257,
                                    gdkcolor->green/257,
                                    gdkcolor->blue/257);

  return ret_str;
}


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel; the gtkColor does not support the rgba color.
 */
GdkColor *
rgba_to_gdkcolor   (gchar  *rgba)
{
  GdkColor *gdkcolor = g_malloc ((gsize) sizeof (GdkColor));
  gchar *rgb = g_strndup (rgba, 6);
  gchar *color = g_strdup_printf ("%s%s", "#", rgb);
  gboolean ret = gdk_color_parse (color, gdkcolor);
  g_free (color);
  g_free (rgb);
  if (!ret)
    {
      g_warning("Unable to parse the color %s", color);
      return NULL;
    }
  return gdkcolor;
}


/* Clear cairo context. */
void
clear_cairo_context     (cairo_t  *cr)
{
  if (cr)
    {
      cairo_save (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_restore (cr);
    }
}


/* Scale the surface with the width and height requested */
cairo_surface_t *
scale_surface      (cairo_surface_t  *surface,
                    gdouble           width,
                    gdouble           height)
{
  gdouble old_width = cairo_image_surface_get_width (surface);
  gdouble old_height = cairo_image_surface_get_height (surface);
	
  cairo_surface_t *new_surface = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
  cairo_t *cr = cairo_create (new_surface);

  /* Scale *before* setting the source surface (1) */
  cairo_scale (cr, width / old_width, height / old_height);
  cairo_set_source_surface (cr, surface, 0, 0);

  /* To avoid getting the edge pixels blended with 0 alpha, which would 
   * occur with the default EXTEND_NONE. Use EXTEND_PAD for 1.2 or newer (2)
   */
  cairo_pattern_set_extend (cairo_get_source(cr), CAIRO_EXTEND_REFLECT); 

  /* Replace the destination with the source instead of overlaying */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  /* Do the actual drawing */
  cairo_paint (cr);
   
  cairo_destroy (cr);

  return new_surface;
}

 
/* Set the cairo surface colour to the RGBA string. */
void
cairo_set_source_color_from_string     ( cairo_t *cr,
                                         gchar   *color)
{
  if (cr)
    {
      guint r,g,b,a;
      sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);

      cairo_set_source_rgba (cr,
                             1.0 * r / 255,
                             1.0 * g /255,
                             1.0 * b /255,
                             1.0 * a /255);

    }
}


/* Save the contents of the pixbuf in the file with name file name. */
gboolean
save_pixbuf_on_png_file      (GdkPixbuf    *pixbuf,
                              const gchar  *filename)
{
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);

  /* Write to the png surface. */
  cairo_surface_write_to_png (surface, filename);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return TRUE;
}


/* Grab the screenshoot and put it in the image buffer. */
GdkPixbuf *
grab_screenshot    ()
{
  gint height = gdk_screen_height ();
  gint width = gdk_screen_width ();

  GdkWindow *root_window = gdk_get_default_root_window ();
  return gdk_pixbuf_get_from_window (root_window,
                                     0,
                                     0,
                                     width,
                                     height);

}


/*
 * This is function return if the point (x,y) in inside the ardesia bar window.
 */
gboolean
inside_bar_window       (gdouble xp,
                         gdouble yp)
{
  gint x = 0;
  gint y = 0;
  gint width = 0;
  gint height = 0;
  gdouble xd = 0;
  gdouble yd = 0;
  GtkWindow *bar = GTK_WINDOW (get_bar_window ());
  gtk_window_get_position (bar, &x, &y);
  xd = (gdouble) x;
  yd = (gdouble) y;

  gtk_window_get_size (bar, &width, &height);

  if ((yp>=yd) && (yp<yd+height))
    {

      if ((xp>=xd) && (xp<xd+width))
        {
          return 1;
        }

    }

  return 0;
}


/* Drill the window in the area where the ardesia bar is located. */
void 
drill_window_in_bar_area     (GtkWidget *widget)
{
  GtkWidget *bar= get_bar_window ();
  gint x, y, width, height;

  gtk_window_get_position (GTK_WINDOW (bar), &x, &y);
  gtk_window_get_size (GTK_WINDOW (bar), &width, &height);

  const cairo_rectangle_int_t widget_rect = { x+1, y+1, width-1, height-1 };               
  cairo_region_t *widget_reg = cairo_region_create_rectangle (&widget_rect);
                                  
  const cairo_rectangle_int_t ann_rect = { 0, 0, gdk_screen_width (), gdk_screen_height () };                        
  cairo_region_t *ann_reg = cairo_region_create_rectangle (&ann_rect);                              
                                  
  cairo_region_subtract (ann_reg, widget_reg);
                                                                                        
  gtk_widget_input_shape_combine_region (widget, ann_reg);
  cairo_region_destroy (ann_reg);
  cairo_region_destroy (widget_reg);
}


/*
 * Get the current date and format in a printable format;
 * the returned value must be free with the g_free.
 */
gchar *
get_date      ()
{
  struct tm *t;
  time_t now;
  gchar *time_sep = ":";
  gchar *date = "";

  time (&now);
  t = localtime (&now);

#ifdef _WIN32
  /* The ":" character on windows is avoided in file name and then
   * I use the "." character instead.
   */
  time_sep = ".";
#endif
  date = g_strdup_printf ("%d-%d-%d_%d%s%d%s%d",
                          t->tm_year+1900,
                          t->tm_mday,
                          t->tm_mon+1,
                          t->tm_hour,
                          time_sep,
                          t->tm_min,
                          time_sep,
                          t->tm_sec);

  return date;
}


/* Return if a file exists. */
gboolean
file_exists        (gchar *filename)
{
  return g_file_test (filename, G_FILE_TEST_EXISTS);
}


/* 
 * Return a file name containing
 * the project name and the current date.
 */
gchar *
get_default_filename    ()
{
  gchar *date = get_date ();
  gchar *filename = g_strdup_printf ("%s_%s", project_name, date);
  g_free (date); 
  return filename;
}


/*
 * Get the home directory.
 */
const gchar *
get_home_dir       (void)
{
  const char *homedir = g_getenv ("HOME");
  if (!homedir)
    {
      homedir = g_get_home_dir ();
    }
  return homedir;
}


/*
 * Get the desktop directory.
 */
const gchar *
get_desktop_dir    (void)
{
  return g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
}


/*
 * Get the documents directory.
 */
const gchar *
get_documents_dir  (void)
{
  const gchar *documents_dir = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
  if (documents_dir == NULL)
    {
       documents_dir = get_home_dir ();
    }
  return documents_dir;
}


/* Delete a directory recursively. */
void
rmdir_recursive    (gchar *path)
{
  GDir *cur_dir;
  const gchar *dir_file;
			
  cur_dir = g_dir_open (path, 0, NULL);

  if (cur_dir)
    {
      while ( (dir_file = g_dir_read_name (cur_dir)))
        {
          gchar *fpath = g_build_filename (path, dir_file, NULL);

          if (fpath)
            {
              if (g_file_test (fpath, G_FILE_TEST_IS_DIR))
                {
                  rmdir_recursive (fpath);
                }
              else
                {
                  g_unlink (fpath);
                }
              g_free (fpath);
            }
        }
			
      g_dir_close (cur_dir);
    }

  g_rmdir (path);
}


/* Remove directory if it is empty */
void
remove_dir_if_empty     (gchar* dir_path)
{
  GDir        *dir = (GDir *) NULL;
  gint         file_occurrence = 0;

  /* if the project dir is empty delete it */
  dir  = g_dir_open (dir_path, 0, NULL);

  while (g_dir_read_name (dir))
    {
      file_occurrence++;
    }

  if (file_occurrence == 0)
    {
      rmdir_recursive(dir_path);
    }
}


/* Allocate a new point belonging to the stroke passing the values. */
AnnotatePoint *
allocate_point     (gdouble  x,
                    gdouble  y,
                    gdouble  width,
                    gdouble  pressure)
{
  AnnotatePoint *point =  g_malloc ( (gsize) sizeof (AnnotatePoint));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  return point;
}


/* Send an email. */
void
send_email         (gchar   *to,
                    gchar   *subject,
                    gchar   *body,
                    GSList  *attachment_list)
{
#ifdef _WIN32
  windows_send_email (to, subject, body, attachment_list);
#else

  guint attach_lenght = g_slist_length (attachment_list);
  guint i = 0;

  gchar* mailer = "xdg-email";
  gchar* subject_param = "--subject";
  gchar* body_param = "--body";
  gchar* attach_param = "--attach";

  gchar* args = g_strdup_printf("%s %s %s %s '%s'", mailer, subject_param, subject, body_param, body);

  for (i=0; i<attach_lenght; i++)
    {
      gchar* attachment = (gchar*) g_slist_nth_data (attachment_list, i);
      gchar* attachment_str = g_strdup_printf("%s '%s'", attach_param, attachment);
      gchar* new_args = g_strdup_printf("%s %s", args, attachment_str);
      g_free(args);
      args = new_args;
      g_free(attachment_str);
    }

  gchar* new_args = g_strdup_printf("%s %s&", args, to);
  g_free(args);

  if (system (new_args)<0)
    {
      g_warning("Problem running command: %s", new_args);
    }

  g_free(new_args);

#endif
}


/* Send artifacts with email. */
void
send_artifacts_with_email    (GSList *attachment_list)
{
  gchar *to = "ardesia-developer@googlegroups.com";
  gchar *subject = "ardesia-contribution";
  gchar *body = g_strdup_printf ("%s,\n%s,%s.",
				 "Dear ardesia developer group", 
				 "I want share my work created with Ardesia with you",
				 "please for details see the attachment" );

  send_email (to, subject, body, attachment_list);
  g_free (body);
}


/* Send trace with email. */
void
send_trace_with_email        (gchar *attachment)
{
  GSList *attachment_list = NULL;
  gchar *to = "ardesia-developer@googlegroups.com";
  gchar *subject = "ardesia-bug-report";

  gchar *body = g_strdup_printf ("%s,\n%s,%s.",
                                 "Dear ardesia developer group", 
                                 "An application error occurred",
                                 "please for details see the attachment with the stack trace" );

  attachment_list = g_slist_prepend (attachment_list, attachment);
  send_email (to, subject, body, attachment_list);
  g_free (body);
}


/* Is the desktop manager gnome. */
gboolean
is_gnome           ()
{
#ifdef _WIN32
  return FALSE;
#endif

  gchar *current_desktop = getenv ("XDG_CURRENT_DESKTOP");
  if (current_desktop)
    {
      if (strcmp (current_desktop, "GNOME")!=0)
        {
          return FALSE;
        }
    }
  return TRUE;
}


/* Create desktop entry passing value. */
void
xdg_create_desktop_entry      (gchar  *filename,
                               gchar  *type,
                               gchar  *name,
                               gchar  *icon,
                               gchar  *exec)
{
  FILE *fp = fopen (filename, "w");
  if (fp)
    {
      fprintf (fp, "[Desktop Entry]\n");
      fprintf (fp, "Type=%s\n", type);
      fprintf (fp, "Name=%s\n", name);
      fprintf (fp, "Icon=%s\n", icon);
      fprintf (fp, "Exec=%s", exec);
      fclose (fp);
      chmod (filename, 0751);
    }
}


/* Create a desktop link. */
void
xdg_create_link         (gchar  *src,
                         gchar  *dest,
                         gchar  *icon)
{
  gchar *link_extension = "desktop";
  gchar *link_filename = g_strdup_printf ("%s.%s", dest, link_extension);

  if (!g_file_test (link_filename, G_FILE_TEST_EXISTS))
    {
       gchar *exec = g_strdup_printf ("xdg-open %s\n", src);
       xdg_create_desktop_entry (link_filename, "Application", PACKAGE_NAME, icon, exec);
       g_free (exec);
    }

  g_free (link_filename);
}


/* Get the last position where sub-string occurs in the string. */
gint
g_substrlastpos         (const char *str,
                         const char *substr)
{
  gint len = (gint) strlen (str);
  gint i = 0;

  for (i = len-1; i >= 0; --i)
    {

      if (str[i] == *substr)
        {
          return i;
        }

    }
  return -1;
}


/* Sub-string of string from start to end position. */
gchar *
g_substr           (const gchar *string,
                    gint         start,
                    gint         end)
{
  gint number_of_char = (end - start + 1);
  gsize size = (gsize) sizeof (gchar) * number_of_char;
  return g_strndup (&string[start], size);
}


/* 
 * This function create a segmentation fault; 
 * it is useful to test the segmentation fault handler.
 */
void create_segmentation_fault    ()
{
  int *f=NULL;
  *f = 0;
}


