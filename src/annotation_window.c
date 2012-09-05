/*
 * Ardesia-- a program for painting on the screen
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY;without even the implied warranty of
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


#include <annotation_window.h>
#include <annotation_window_callbacks.h>
#include <utils.h>
#include <input.h>
#include <broken.h>
#include <background_window.h>
#include <bezier_spline.h>
#include <cursors.h>
#include <iwb_loader.h>
#include <iwb_saver.h>


#ifdef _WIN32
#  include <windows_utils.h>
#endif


/* Internal data for the annotation window. */
static AnnotateData *data;


/* Create a new paint context. */
static AnnotatePaintContext *
annotate_paint_context_new        (AnnotatePaintType type)
{
  AnnotatePaintContext *context = (AnnotatePaintContext *) NULL;
  gchar* color = g_strdup ("FF0000FF");

  context = g_malloc ((gsize) sizeof (AnnotatePaintContext));
  context->fg_color = color;
  context->type = type;

  return context;
}


/* Calculate the direction in radiant. */
static gdouble
annotate_get_arrow_direction      (AnnotateDeviceData *devdata)
{
  /* Precondition: the list must be not null and the length might be greater than two. */
  AnnotatePoint *point = (AnnotatePoint *) NULL;
  AnnotatePoint *old_point = (AnnotatePoint *) NULL;
  gdouble delta = 2.0;
  gdouble ret = 0.0;
  GSList *out_ptr = devdata->coord_list;
  gdouble tollerance = annotate_get_thickness () * delta;

  /* Build the relevant point list with the standard deviation algorithm. */
  GSList *relevantpoint_list = build_meaningful_point_list (out_ptr, FALSE, tollerance);

  old_point = (AnnotatePoint *) g_slist_nth_data (relevantpoint_list, 1);
  point = (AnnotatePoint *) g_slist_nth_data (relevantpoint_list, 0);
  /* Give the direction using the last two point. */
  ret = atan2 (point->y-old_point->y, point->x-old_point->x);

  /* Free the relevant point list. */
  g_slist_foreach (relevantpoint_list, (GFunc) g_free, (gpointer) NULL);
  g_slist_free (relevantpoint_list);
  relevantpoint_list = (GSList *) NULL;

  return ret;
}


/* Colour selector; if eraser than select the transparent colour else allocate the right colour. */
static void
select_color            ()
{
  if (!data->annotation_cairo_context)
    {
      return;
    }

  if (data->cur_context)
    {
      if (data->cur_context->type != ANNOTATE_ERASER) //pen or arrow tool
        {
          /* Select the colour. */
          if (data->cur_context->fg_color)
            {
              if (data->debug)
                {
                  g_printerr ("Select colour %s\n", data->cur_context->fg_color);
                }

              cairo_set_source_color_from_string (data->annotation_cairo_context,
                                                  data->cur_context->fg_color);
            }

          cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
        }
      else
        {

          /* It is the eraser tool. */
          if (data->debug)
            {
              g_printerr ("The eraser tool has been selected\n");
            }

          cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_CLEAR);
        }
    }
}


#ifdef _WIN32


/* Acquire the grab pointer. */
static void
annotate_acquire_pointer_grab ()
{
  grab_pointer (data->annotation_window, GDK_ALL_EVENTS_MASK);
}


/* Release the grab pointer. */
static void
annotate_release_pointer_grab     ()
{
  ungrab_pointer (gdk_display_get_default ());
}

#endif


/* Update the cursor icon. */
static void
update_cursor      ()
{
  if (!data->annotation_window)
    {
      return;
    }

#ifdef _WIN32
  annotate_release_pointer_grab ();
#endif

  gdk_window_set_cursor (gtk_widget_get_window (data->annotation_window), data->cursor);

#ifdef _WIN32
  annotate_acquire_pointer_grab ();
#endif
}


/* Dis-allocate cursor. */
static void
disallocate_cursor     ()
{
  if (data->cursor)
    {
      g_object_unref (data->cursor);
      data->cursor = (GdkCursor *) NULL;
    }
}


/* Take the input mouse focus. */
static void
annotate_acquire_input_grab  ()
{
#ifdef _WIN32
  grab_pointer (data->annotation_window, GDK_ALL_EVENTS_MASK);
#endif

#ifndef _WIN32
  /*
   * MACOSX; will do nothing.
   */
  gtk_widget_input_shape_combine_region(data->annotation_window, NULL);
  drill_window_in_bar_area (data->annotation_window);
#endif

}


/* Destroy cairo context. */
static void
destroy_cairo           ()
{
  guint refcount =  (guint) cairo_get_reference_count (data->annotation_cairo_context);

  guint i = 0;

  for  (i=0; i<refcount; i++)
    {
      cairo_destroy (data->annotation_cairo_context);
    }

  data->annotation_cairo_context = (cairo_t *) NULL;
}


/* This an ellipse taking the top left edge coordinates
 * and the width and the height of the bounded rectangle.
 */
static void
annotate_draw_ellipse   (AnnotateDeviceData *devdata,
                         gdouble x,
                         gdouble y,
                         gdouble width,
                         gdouble height,
                         gdouble pressure)
{
  if (data->debug)
    {
      g_printerr ("Draw ellipse: 2a=%f 2b=%f\n", width, height);
    }

  annotate_modify_color (devdata, data, pressure);

  cairo_save (data->annotation_cairo_context);

  /* The ellipse is done as a 360 degree arc translated. */
  cairo_translate (data->annotation_cairo_context, x + width / 2., y + height / 2.);
  cairo_scale (data->annotation_cairo_context, width / 2., height / 2.);
  cairo_arc (data->annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore (data->annotation_cairo_context);

}


/* Return if it is a closed path. */
static gboolean
is_a_closed_path        (GSList* list)
{
  /* Check if it is a closed path. */
  guint lenght = g_slist_length (list);
  AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (list, 0);
  AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (list, lenght-1);

  gint distance = (gint) get_distance (first_point->x,
                                       first_point->y,
                                       last_point->x,
                                       last_point->y);

  if ( distance == 0)
    {
      return TRUE;
    }
  return FALSE;
}


/* Draw the point list;the cairo path is not forgotten */
static void
annotate_draw_point_list     (AnnotateDeviceData *devdata,
                              GSList             *list)
{
  if (list)
    {
      guint i = 0;
      guint lenght = g_slist_length (list);
      for (i=0; i<lenght; i=i+1)
        {
          AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
          if (!point)
            {
              return;
            }
            
          if (lenght == 1)
            {
              /* It is a point. */
              annotate_draw_point (devdata, point->x, point->y, point->pressure);
              break;
            }
          annotate_modify_color (devdata, data, point->pressure);
          /* Draw line between the two points. */
          annotate_draw_line (devdata, point->x, point->y, FALSE);
        }
    }
}


/* Draw a curve using a cubic bezier splines passing to the list's coordinate. */
static void
annotate_draw_curve    (AnnotateDeviceData *devdata,
                        GSList             *list)
{
  guint lenght = g_slist_length (list);

  if (list)
    {
      guint i = 0;
      for (i=0; i<lenght; i=i+3)
        {
          AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (list, i);
          if (!first_point)
            {
              return;
            }
          if (lenght == 1)
            {
              /* It is a point. */
              annotate_draw_point (devdata, first_point->x, first_point->y, first_point->pressure);
            }
          else
            {
              AnnotatePoint *second_point = (AnnotatePoint *) g_slist_nth_data (list, i+1);
              if (!second_point)
                {
                  return;
                }
              else
                {
                  AnnotatePoint *third_point = (AnnotatePoint *) g_slist_nth_data (list, i+2);
                  if (!third_point)
                    {
                      /* draw line from first to second point */
                      annotate_draw_line (devdata, second_point->x, second_point->y, FALSE);
                      return;
                    }
                  annotate_modify_color (devdata, data, second_point->pressure);
                  cairo_curve_to (data->annotation_cairo_context,
                                  first_point->x,
                                  first_point->y,
                                  second_point->x,
                                  second_point->y,
                                  third_point->x,
                                  third_point->y);
                }
            }
        }
    }
}


/* Rectify the line. */
static void
rectify            (AnnotateDeviceData *devdata,
                    gboolean closed_path)
{
  gdouble tollerance = annotate_get_thickness ();
  GSList *broken_list = broken (devdata->coord_list, closed_path, TRUE, tollerance);

  if (data->debug)
    {
      g_printerr ("rectify\n");
    }

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  annotate_draw_point_list (devdata, broken_list);

  annotate_coord_dev_list_free (devdata);
  devdata->coord_list = broken_list;
  
}


/* Roundify the line. */
static void
roundify           (AnnotateDeviceData *devdata,
                    gboolean closed_path)
{
  gdouble tollerance = annotate_get_thickness ();

  /* Build the meaningful point list with the standard deviation algorithm. */
  GSList *meaningful_point_list = (GSList *) NULL;

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  meaningful_point_list = build_meaningful_point_list (devdata->coord_list, closed_path, tollerance);

  if ( g_slist_length (meaningful_point_list) < 4)
    {
      /* Draw the point line as is and jump the bezier algorithm. */
      annotate_draw_point_list (devdata, meaningful_point_list);
    }
  else if ((closed_path) && (is_similar_to_an_ellipse (meaningful_point_list, tollerance)))
    {
      GSList *rect_list = build_outbounded_rectangle (meaningful_point_list);

      if (rect_list)
        {
          AnnotatePoint *point1 = (AnnotatePoint *) g_slist_nth_data (rect_list, 0);
          AnnotatePoint *point2 = (AnnotatePoint *) g_slist_nth_data (rect_list, 1);
          AnnotatePoint *point3 = (AnnotatePoint *) g_slist_nth_data (rect_list, 2);
          gdouble p1p2 = get_distance(point1->x, point1->y, point2->x, point2->y);
          gdouble p2p3 = get_distance(point2->x, point2->y, point3->x, point3->y);
          gdouble e_threshold = 0.5;
          gdouble a = 0;
          gdouble b = 0;
          if (p1p2>p2p3)
            {
              b = p2p3/2;
              a = p1p2/2;
            }
          else
            {
              a = p2p3/2;
              b = p1p2/2;
            }
          gdouble e = 1-powf((b/a), 2);
          /* If the eccentricity is roundable to 0 it is a circle */
          if ((e >= 0) && (e <= e_threshold))
            {
              /* Move the down right point in the right position to square the circle */
              gdouble quad_distance = (p1p2+p2p3)/2;
              point3->x = point1->x+quad_distance;
              point3->y = point1->y+quad_distance;
            }

          annotate_draw_ellipse (devdata, point1->x, point1->y, point3->x-point1->x, point3->y-point1->y, point1->pressure);
          g_slist_foreach (rect_list, (GFunc)g_free, NULL);
          g_slist_free (rect_list);
        }
    }

  else
    {
      /* It is not an ellipse; I use bezier to spline the path. */
      GSList *splined_list = spline (meaningful_point_list);
      annotate_draw_curve (devdata, splined_list);

      annotate_coord_dev_list_free (devdata);
      devdata->coord_list = splined_list;

    }

  g_slist_foreach (meaningful_point_list, (GFunc) g_free, (gpointer) NULL);
  g_slist_free (meaningful_point_list);
}


/* Create the annotation window. */
static GtkWidget *
create_annotation_window     ()
{
  GtkWidget* widget = (GtkWidget *) NULL;
  GError* error = (GError *) NULL;

  /* Initialize the main window. */
  data->annotation_window_gtk_builder = gtk_builder_new ();

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (data->annotation_window_gtk_builder, ANNOTATION_UI_FILE, &error);

  if (error)
    {
      g_warning ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      return widget;
    }

  widget = GTK_WIDGET (gtk_builder_get_object (data->annotation_window_gtk_builder,
                       "annotationWindow"));

  return widget;
}


/* Set-up the application. */
static void
setup_app          (GtkWidget* parent)
{
  gint width = gdk_screen_width ();
  gint height = gdk_screen_height ();

  /* Create the annotation window. */
  data->annotation_window = create_annotation_window ();
 
  /* This trys to set an alpha channel. */
  on_screen_changed(data->annotation_window, NULL, data);
  
  /* In the gtk 2.16.6 the gtkbuilder property double-buffered is not parsed from the glade file
   * and then I set this by hands.
   */
  gtk_widget_set_double_buffered (data->annotation_window, FALSE);

  /* Put the opacity to 0 to avoid the initial flickering. */
  gtk_window_set_opacity (GTK_WINDOW (data->annotation_window), 0.0);

  if (data->annotation_window == NULL)
    {
      g_warning ("Failed to create the annotation window");
      return;
    }

  //gtk_window_set_transient_for (GTK_WINDOW (data->annotation_window), GTK_WINDOW (parent));

  gtk_widget_set_size_request (data->annotation_window, width, height);

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals (data->annotation_window_gtk_builder, (gpointer) data);

  /* Connet some extra callbacks in order to handle the hotplugged input devices. */
  g_signal_connect (gdk_display_get_device_manager (gdk_display_get_default ()),
                    "device-added",
                    G_CALLBACK (on_device_added),
                    data);
  g_signal_connect (gdk_display_get_device_manager (gdk_display_get_default ()),
                    "device-removed",
                    G_CALLBACK (on_device_removed),
                    data);

  /* This show the window in fullscreen generating an exposure. */
  gtk_window_fullscreen (GTK_WINDOW (data->annotation_window));

#ifdef _WIN32
  /* @TODO Use RGBA colormap and avoid to use the layered window. */
  /* I use a layered window that use the black as transparent colour. */
  setLayeredGdkWindowAttributes (gtk_widget_get_window(data->annotation_window),
                                 RGB (0,0,0),
                                 0,
                                 LWA_COLORKEY);	
#endif
}


/* Create the directory where put the save-point files. */
static void
create_savepoint_dir    ()
{
  const gchar *tmpdir = g_get_tmp_dir ();
  gchar *images = "images";
  gchar *project_name = get_project_name ();
  gchar *ardesia_tmp_dir = g_build_filename (tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar *project_tmp_dir = g_build_filename (ardesia_tmp_dir, project_name, (gchar *) 0);

  if (g_file_test (ardesia_tmp_dir, G_FILE_TEST_IS_DIR))
    {
      /* The folder already exist;I delete it. */
      rmdir_recursive (ardesia_tmp_dir);
    }

  data->savepoint_dir = g_build_filename (project_tmp_dir, images, (gchar *) 0);
  g_mkdir_with_parents (data->savepoint_dir, 0777);
  g_free (ardesia_tmp_dir);
  g_free (project_tmp_dir);
}


/* Delete the save-point. */
static void
delete_savepoint        (AnnotateSavepoint *savepoint)
{
  if (savepoint)
    {

      if (data->debug)
        {
          g_printerr ("The save-point %s has been removed\n", savepoint->filename);
        }

      if (savepoint->filename)
        {
          g_remove (savepoint->filename);
          g_free (savepoint->filename);
          savepoint->filename = (gchar *) NULL;
        }
      data->savepoint_list = g_slist_remove (data->savepoint_list, savepoint);
      g_free (savepoint);
      savepoint = (AnnotateSavepoint *) NULL;
    }
}


/* Free the list of the  save-point for the redo. */
static void
annotate_redolist_free       ()
{
  guint i = data->current_save_index;
  GSList *stop_list = g_slist_nth (data->savepoint_list, i);
  
  while (data->savepoint_list != stop_list)
    {
      AnnotateSavepoint *savepoint = (AnnotateSavepoint *) g_slist_nth_data (data->savepoint_list, 0);
      delete_savepoint (savepoint);
    }
}


/* Free the list of all the save-point. */
static void
annotate_savepoint_list_free ()
{
  while (data->savepoint_list)
    {
      AnnotateSavepoint *savepoint = (AnnotateSavepoint *) g_slist_nth_data (data->savepoint_list, 0);
      delete_savepoint (savepoint);
    }

  data->savepoint_list = (GSList *) NULL;
}


/* Delete the ardesia temporary directory */
static void 
delete_ardesia_tmp_dir       ()
{
  gchar *ardesia_tmp_dir = g_build_filename (g_get_tmp_dir (), PACKAGE_NAME, (gchar *) 0);
  rmdir_recursive (ardesia_tmp_dir);
  g_free (ardesia_tmp_dir);
}


/* Draw an arrow starting from the point 
 * whith the width and the direction in radiant
 */
static void
draw_arrow_in_point     (AnnotatePoint      *point,
                         gdouble             width,
                         gdouble             direction)
{

  gdouble width_cos = width * cos (direction);
  gdouble width_sin = width * sin (direction);

  /* Vertex of the arrow. */
  gdouble arrow_head_0_x = point->x + width_cos;
  gdouble arrow_head_0_y = point->y + width_sin;

  /* Left point. */
  gdouble arrow_head_1_x = point->x - width_cos + width_sin;
  gdouble arrow_head_1_y = point->y -  width_cos - width_sin;

  /* Origin. */
  gdouble arrow_head_2_x = point->x - 0.8 * width_cos;
  gdouble arrow_head_2_y = point->y - 0.8 * width_sin;

  /* Right point. */
  gdouble arrow_head_3_x = point->x - width_cos - width_sin;
  gdouble arrow_head_3_y = point->y +  width_cos - width_sin;

  cairo_stroke (data->annotation_cairo_context);
  cairo_save (data->annotation_cairo_context);

  /* Initialize cairo properties. */
  cairo_set_line_join (data->annotation_cairo_context, CAIRO_LINE_JOIN_MITER);
  cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width (data->annotation_cairo_context, width);

  /* Draw the arrow. */
  cairo_move_to (data->annotation_cairo_context, arrow_head_2_x, arrow_head_2_y);
  cairo_line_to (data->annotation_cairo_context, arrow_head_1_x, arrow_head_1_y);
  cairo_line_to (data->annotation_cairo_context, arrow_head_0_x, arrow_head_0_y);
  cairo_line_to (data->annotation_cairo_context, arrow_head_3_x, arrow_head_3_y);

  cairo_close_path (data->annotation_cairo_context);
  cairo_fill_preserve (data->annotation_cairo_context);
  cairo_stroke (data->annotation_cairo_context);
  cairo_restore (data->annotation_cairo_context);

  if (data->debug)
    {
      g_printerr ("with vertex at (x,y)= (%f : %f)\n",  arrow_head_0_x , arrow_head_0_y);
    }
}


/* Fill the shape drawn by the devdata owner. */
static void fill_dev    (AnnotateDeviceData *devdata)
{

  if (devdata->coord_list != NULL)
    {

      /* if is not a closed path I prevent to fill it */
      if (!is_a_closed_path (devdata->coord_list))
        {
          if (data->debug)
            {
              g_printerr ("Fill is not done; no closed path has been detected\n");
            }
          return;
        }

      if ( !(data->roundify) && (!(data->rectify)))
        {
          annotate_draw_point_list (devdata, devdata->coord_list);
        }

      cairo_close_path (data->annotation_cairo_context);
      select_color (devdata);
      cairo_fill (data->annotation_cairo_context);
      cairo_stroke (data->annotation_cairo_context);

      if (data->debug)
        {
          g_printerr ("Fill\n");
        }

      annotate_add_savepoint ();
    }
}


/* Configure pen option for cairo context. */
void 
annotate_configure_pen_options    (AnnotateData       *data)
{

  if (data->annotation_cairo_context)
    {
      cairo_new_path (data->annotation_cairo_context);
      cairo_set_line_cap(data->annotation_cairo_context, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_ROUND);
      
      if (data->old_paint_type == ANNOTATE_ERASER)
        {
          data->cur_context = data->default_eraser;
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_CLEAR);
          cairo_set_line_width(data->annotation_cairo_context, annotate_get_thickness ());
        }
      else
        {
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
          cairo_set_line_width(data->annotation_cairo_context, annotate_get_thickness ());
        }
    }
    select_color ();
}


/*
 * Add a save point for the undo/redo;
 * this code must be called at the end of each painting action.
 */
void
annotate_add_savepoint  ()
{
  AnnotateSavepoint *savepoint = g_malloc ((gsize) sizeof (AnnotateSavepoint));
  cairo_surface_t *saved_surface = (cairo_surface_t *) NULL;
  cairo_surface_t *source_surface = (cairo_surface_t *) NULL;
  cairo_t *cr = (cairo_t *) NULL;

  /* The story about the future is deleted. */
  annotate_redolist_free ();

  guint savepoint_index = g_slist_length (data->savepoint_list) + 1;
  
  savepoint->filename = g_strdup_printf ("%s%s%s_%d_vellum.png",
                                          data->savepoint_dir,
                                          G_DIR_SEPARATOR_S,
                                          PACKAGE_NAME,
                                          savepoint_index);

  /* Add a new save-point. */
  data->savepoint_list = g_slist_prepend (data->savepoint_list, savepoint);
  data->current_save_index = 0;

  /* Load a surface with the data->annotation_cairo_context content and write the file. */
  saved_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                              gdk_screen_width (),
                                              gdk_screen_height ());

  source_surface = cairo_get_target (data->annotation_cairo_context);
  cr = cairo_create (saved_surface);
  cairo_set_source_surface (cr, source_surface, 0, 0);
  cairo_paint (cr);
  /* Postcondition: the saved_surface now contains the save-point image. */


  /*  Will be create a file in the save-point folder with format PACKAGE_NAME_1.png. */
  cairo_surface_write_to_png (saved_surface, savepoint->filename);
  cairo_surface_destroy (saved_surface);
  if (data->debug)
    {
      g_printerr ("The save point %s has been stored in file\n", savepoint->filename);
    }

  cairo_destroy (cr);
}


/* Initialize the annotation cairo context */
void
initialize_annotation_cairo_context    (AnnotateData *data)
{
  if (data->annotation_cairo_context == NULL)
    {
      /* Initialize a transparent window. */
#ifdef _WIN32
      /* The hdc has depth 32 and the technology is DT_RASDISPLAY. */
      HDC hdc = GetDC (GDK_WINDOW_HWND (gtk_widget_get_window (data->annotation_window)));
      /* 
       * @TODO Use an HDC that support the ARGB32 format to support the alpha channel;
       * this might fix the highlighter bug.
       * In the documentation is written that the now the resulting surface is in RGB24 format.
       * 
       */
      cairo_surface_t *surface = cairo_win32_surface_create (hdc);

      data->annotation_cairo_context = cairo_create (surface);
#else
      data->annotation_cairo_context = gdk_cairo_create (gtk_widget_get_window (data->annotation_window));
#endif

  if (cairo_status (data->annotation_cairo_context) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Failed to allocate the annotation cairo context"); 
      annotate_quit ();
      exit (EXIT_FAILURE);
    }

  if (data->savepoint_list == NULL)
    {
      /* Clear the screen and create the first empty savepoint. */
      annotate_clear_screen ();
    }
#ifndef _WIN32
      gtk_window_set_opacity(GTK_WINDOW(data->annotation_window), 1.0);
#endif	
   annotate_acquire_grab ();

    }
}


/* Draw the last save point on the window restoring the surface. */
void
annotate_restore_surface     ()
{

  if (data->debug)
    {
      g_printerr ("Restore surface\n");
    }

  if (data->annotation_cairo_context)
    {
      guint i = data->current_save_index;

      if (g_slist_length (data->savepoint_list)==i)
        {
          cairo_new_path (data->annotation_cairo_context);
          clear_cairo_context (data->annotation_cairo_context);
          return;
        }

      AnnotateSavepoint *savepoint = (AnnotateSavepoint *) g_slist_nth_data (data->savepoint_list, i);

      if (!savepoint)
        {
          return;
        }

      cairo_new_path (data->annotation_cairo_context);
      cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);

      if (savepoint->filename)
        {
          /* Load the file in the annotation surface. */
          cairo_surface_t *image_surface = cairo_image_surface_create_from_png(savepoint->filename);
          if (data->debug)
            {
              g_printerr ("The save-point %s has been loaded from file\n", savepoint->filename);
            }

          if (image_surface)
            {
              cairo_set_source_surface(data->annotation_cairo_context, image_surface, 0, 0);
              cairo_paint (data->annotation_cairo_context);
              cairo_stroke (data->annotation_cairo_context);
              cairo_surface_destroy(image_surface);
            }

        }
    }
}


/* Get the annotation window. */
GtkWidget *
get_annotation_window   ()
{
  return data->annotation_window;
}


/* Set colour. */
void
annotate_set_color      (gchar      *color)
{
  data->cur_context->fg_color = color;
}


/* Set rectifier. */
void
annotate_set_rectifier  (gboolean  rectify)
{
  data->rectify = rectify;
}


/* Set rounder. */
void
annotate_set_rounder    (gboolean roundify)
{
  data->roundify = roundify;
}


/* Set arrow. */
void
annotate_set_arrow      (gboolean    arrow)
{
  data->arrow = arrow;
}


/* Set the line thickness. */
void
annotate_set_thickness  (gdouble thickness)
{
  data->thickness = thickness;
}


/* Get the line thickness. */
gdouble
annotate_get_thickness  ()
{
  if (data->cur_context->type == ANNOTATE_ERASER)
    {
      /* the eraser is bigger than pen */
      gdouble corrective_factor = 2.5;
      return data->thickness * corrective_factor;
    }

  return data->thickness;
}


/* Add to the list of the painted point the point (x,y). */
void
annotate_coord_list_prepend  (AnnotateDeviceData  *devdata,
                              gdouble              x,
                              gdouble              y,
                              gdouble              width,
                              gdouble              pressure)
{
  AnnotatePoint *point = g_malloc ((gsize) sizeof (AnnotatePoint));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  devdata->coord_list = g_slist_prepend (devdata->coord_list, point);
}


/* Free the coord list belonging to the the owner devdata device. */
void
annotate_coord_dev_list_free (AnnotateDeviceData *devdata)
{
  if (devdata->coord_list)
    {
      g_slist_foreach (devdata->coord_list, (GFunc) g_free, (gpointer) NULL);
      g_slist_free (devdata->coord_list);
      devdata->coord_list = (GSList *) NULL;
    }
}


/* Modify colour according to the pressure. */
void
annotate_modify_color   (AnnotateDeviceData *devdata,
                         AnnotateData       *data,
                         gdouble             pressure)
{
  /* Pressure value is from 0 to 1;this value modify the RGBA gradient. */
  guint r,g,b,a;
  gdouble old_pressure = pressure;

  /* If you put an higher value you will have more contrast
   * between the lighter and darker colour depending on pressure.
   */
  gdouble contrast = 96;
  gdouble corrective = 0;
  
  /* The pressure is greater than 0. */
  if ( (!data->annotation_cairo_context) || (!data->cur_context->fg_color))
    {
      return;
    }

  if (pressure >= 1)
    {
      cairo_set_source_color_from_string (data->annotation_cairo_context,
                                          data->cur_context->fg_color);
    }
  else if (pressure <= 0.1)
    {
      pressure = 0.1;
    }

  sscanf (data->cur_context->fg_color, "%02X%02X%02X%02X", &r, &g, &b, &a);

  if (devdata->coord_list != NULL)
    {
      AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);
      old_pressure = last_point->pressure;
    }

  corrective = (1- ( 3 * pressure + old_pressure)/4) * contrast;
  cairo_set_source_rgba (data->annotation_cairo_context,
                         (r + corrective)/255,
                         (g + corrective)/255,
                         (b+corrective)/255,
                         (gdouble) a/255);
}

/* Paint the context over the annotation window. */
void
annotate_push_context (cairo_t * cr)
{
  cairo_surface_t* source_surface = (cairo_surface_t *) NULL;

  if (data->debug)
    {
      g_printerr ("The text window content has been painted over the annotation window\n");
    }

  cairo_new_path (data->annotation_cairo_context);
  source_surface = cairo_get_target (cr);

#ifndef _WIN32
  /*
   * The over operator might put the new layer on the top of the old one
   * overriding the intersections
   * Seems that this operator does not work on windows
   * in this operating system only the new layer remain after the merge.
   *
   */
  cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_OVER);
#else
  /*
   * @WORKAROUND using CAIRO_OPERATOR_ADD instead of CAIRO_OPERATOR_OVER
   * I do this to use the text widget in windows 
   * I use the CAIRO_OPERATOR_ADD that put the new layer
   * on the top of the old;this function does not preserve the colour of
   * the second layer but modify it respecting the first layer.
   *
   * Seems that the CAIRO_OPERATOR_OVER does not work because in the
   * gtk cairo implementation the ARGB32 format is not supported.
   *
   */
  cairo_set_operator (data->annotation_cairo_context, CAIRO_OPERATOR_ADD);
#endif

  cairo_set_source_surface (data->annotation_cairo_context, source_surface, 0, 0);
  cairo_paint (data->annotation_cairo_context);
  cairo_stroke (data->annotation_cairo_context);
  annotate_add_savepoint ();
}


/* Select the default pen tool. */
void
annotate_select_pen     ()
{
  if (data->debug)
    {
      g_printerr ("The pen with colour %s has been selected\n",
                  data->cur_context->fg_color);
    }

  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;

  disallocate_cursor ();

  set_pen_cursor (&data->cursor,
                  data->thickness,
                  data->cur_context->fg_color,
                  data->arrow);

  update_cursor ();
}


/* Select the default eraser tool. */
void
annotate_select_eraser  ()
{
  if (data->debug)
    {
      g_printerr ("The eraser has been selected\n");
    }

  data->cur_context = data->default_eraser;
  data->old_paint_type = ANNOTATE_ERASER;

  disallocate_cursor ();

  set_eraser_cursor (&data->cursor, annotate_get_thickness ());

  update_cursor ();
}


/* Unhide the cursor. */
void
annotate_unhide_cursor ()
{
  if (data->is_cursor_hidden)
    {
      update_cursor ();
      data->is_cursor_hidden = FALSE;
    }
}


/* Hide the cursor icon. */
void
annotate_hide_cursor    ()
{
  gdk_window_set_cursor (gtk_widget_get_window (data->annotation_window),
                         data->invisible_cursor);

  data->is_cursor_hidden = TRUE;
}


/* acquire the grab. */
void
annotate_acquire_grab   ()
{
  ungrab_pointer     (gdk_display_get_default ());
  if  (!data->is_grabbed)
    {

      if (data->debug)
        {
          g_printerr ("Acquire grab\n");
        }

      annotate_acquire_input_grab ();
      data->is_grabbed = TRUE;
    }
}


/* Draw line from the last point drawn to (x2,y2);
 * if stroke is false the cairo path is not forgotten
 */
void
annotate_draw_line      (AnnotateDeviceData  *devdata,
                         gdouble              x2,
                         gdouble              y2,
                         gboolean             stroke)
{
  if (!stroke)
    {
      cairo_line_to (data->annotation_cairo_context, x2, y2);
    }
  else
    {
      AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);
      cairo_move_to (data->annotation_cairo_context, last_point->x, last_point->y);
      cairo_line_to (data->annotation_cairo_context, x2, y2);
      cairo_stroke (data->annotation_cairo_context);
    }
}


/* Draw an arrow using some polygons. */
void
annotate_draw_arrow     (AnnotateDeviceData  *devdata,
                         gdouble              distance)
{
  gdouble direction = 0;
  gdouble pen_width = annotate_get_thickness ();
  gdouble arrow_minimum_size = pen_width * 2;

  AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);

  if (distance < arrow_minimum_size)
    {
      return;
    }

  if (data->debug)
    {
      g_printerr ("Draw arrow: ");
    }

  if (g_slist_length (devdata->coord_list) < 2)
    {
      /* If it has length lesser then two then is a point and it has no sense draw the arrow. */
      return;
    }

  /* Postcondition length >= 2 */
  direction = annotate_get_arrow_direction (devdata);

  if (data->debug)
    {
      g_printerr ("Arrow direction %f\n", direction/M_PI*180);
    }

  draw_arrow_in_point(point, pen_width, direction);
}


/* Draw a point in x,y respecting the context. */
void
annotate_draw_point     (AnnotateDeviceData  *devdata,
                         gdouble              x,
                         gdouble              y,
                         gdouble              pressure)
{
  /* Modify a little bit the colour depending on pressure. */
  annotate_modify_color (devdata, data, pressure);
  cairo_move_to (data->annotation_cairo_context, x, y);
  cairo_line_to (data->annotation_cairo_context, x, y);
}


/* Call the geometric shape recognizer. */
void
annotate_shape_recognize     (AnnotateDeviceData  *devdata,
                              gboolean             closed_path)
{
  if (data->rectify)
    {
      rectify (devdata, closed_path);
    }
  else if (data->roundify)
    {
      roundify (devdata, closed_path);
    }
}


/* Select eraser, pen or other tool for tablet. */
void
annotate_select_tool    (AnnotateData  *data,
                         GdkDevice     *device,
                         guint state)
{
  if (device)
    {
      if (gdk_device_get_source (device) == GDK_SOURCE_ERASER)
        {
          annotate_select_eraser ();
        }
      else
        {
          annotate_select_pen ();
        }

    }
  else
    {
      g_printerr ("Attempt to select non existent device!\n");
    }
}


/* Free the memory allocated by paint context */
void
annotate_paint_context_free (AnnotatePaintContext *context)
{
  if (context)
    {
      g_free (context);
      context = (AnnotatePaintContext *) NULL;
    }
}


/* Quit the annotation. */
void
annotate_quit           ()
{
  export_iwb (get_iwb_filename ());
  if (data)
    {
	
      /* Destroy cairo object. */
      destroy_cairo ();

      /* Destroy cursors. */
      disallocate_cursor ();
      cursors_main_quit ();

      if (data->invisible_cursor)
        {
          g_object_unref (data->invisible_cursor);
          data->invisible_cursor = (GdkCursor *) NULL;
        }

      /* Free all. */
      if (data->annotation_window)
        {
          gtk_widget_destroy (data->annotation_window);
          data->annotation_window = (GtkWidget *) NULL;
        }
  
      remove_input_devices (data);
      annotate_savepoint_list_free ();

      delete_ardesia_tmp_dir();

      g_free (data->savepoint_dir);
      data->savepoint_dir = (gchar *) NULL;

      annotate_paint_context_free (data->default_pen);
      data->default_pen = (AnnotatePaintContext *) NULL;

      annotate_paint_context_free (data->default_eraser);
      data->default_eraser = (AnnotatePaintContext *) NULL;

    }
}


/* Release input grab;the input event will be passed below the window. */
void
annotate_release_input_grab  ()
{
  ungrab_pointer (gdk_display_get_default ());
  //gdk_window_set_cursor (gtk_widget_get_window (data->annotation_window), (GdkCursor *) NULL);
#ifndef _WIN32
  /*
   * @TODO implement correctly gtk_widget_input_shape_combine_mask
   * in the quartz gdkwindow or use an equivalent native function;
   * the current implementation in macosx this does not do nothing.
   */
  /*
   * This allows the mouse event to be passed below the transparent annotation;
   * at the moment this call works only on Linux
   */
  gtk_widget_input_shape_combine_region(data->annotation_window, NULL);
  
  const cairo_rectangle_int_t ann_rect = { 0, 0, 0, 0 };
  cairo_region_t *r = cairo_region_create_rectangle (&ann_rect);
  
  gtk_widget_input_shape_combine_region (data->annotation_window, r);
  cairo_region_destroy (r);
  
#else
  /*
   * @TODO WIN32 implement correctly gtk_widget_input_shape_combine_mask
   * in the win32 gdkwindow or use an equivalent native function.
   * Now in the gtk implementation the gtk_widget_input_shape_combine_mask
   * call the gtk_widget_shape_combine_mask that is not the desired behaviour.
   *
   */
   
#endif
}


/* Release the pointer pointer. */
void
annotate_release_grab   ()
{
  if (data->is_grabbed)
    {

      if (data->debug)
        {
          g_printerr ("Release grab\n");
        }

      annotate_release_input_grab ();
      gtk_window_present (GTK_WINDOW (get_bar_window ()));
      data->is_grabbed = FALSE;
    }
}


/* Fill the last shapes. */
void annotate_fill      ()
{
  if (data->debug)
    {
      g_printerr ("Fill\n");
    }
    
  GHashTableIter it;
  gpointer value;
  g_hash_table_iter_init (&it, data->devdatatable);
  
  while (g_hash_table_iter_next (&it, NULL, &value))
    {
       AnnotateDeviceData *devdata = (AnnotateDeviceData *) value;
       fill_dev (devdata);
    }
}


/* Undo reverting to the last save point. */
void
annotate_undo           ()
{
  if (data->debug)
    {
      g_printerr ("Undo\n");
    }

  if (data->savepoint_list)
    {
      if (data->current_save_index != g_slist_length (data->savepoint_list)-1)
        {
          data->current_save_index = data->current_save_index + 1;
          annotate_restore_surface ();
        }
    }
}


/* Redo to the last save point. */
void
annotate_redo           ()
{
  if (data->debug)
    {
      g_printerr ("Redo\n");
    }

  if (data->savepoint_list)
    {
      if (data->current_save_index != 0)
        {
          data->current_save_index = data->current_save_index - 1;
          annotate_restore_surface ();
        }
    }
}


/* Clear the annotations windows and make an empty savepoint. */
void
annotate_clear_screen   ()
{
  if (data->debug)
    {
      g_printerr ("Clear screen\n");
    }

  cairo_new_path (data->annotation_cairo_context);
  clear_cairo_context (data->annotation_cairo_context);
  gtk_widget_queue_draw_area (data->annotation_window, 0, 0, gdk_screen_width (), gdk_screen_height ());
  /* Add the empty savepoint. */
  annotate_add_savepoint ();
}


/* Initialize the annotation. */
gint 
annotate_init                (GtkWidget  *parent,
                              gchar      *iwb_file,
                              gboolean    debug)
{
  cursors_main ();
  data = g_malloc ((gsize) sizeof (AnnotateData));

  /* Initialize the data structure. */
  data->annotation_cairo_context = (cairo_t *) NULL;
  data->savepoint_list = (GSList *) NULL;
  data->current_save_index = 0;
  data->cursor = (GdkCursor *) NULL;
  data->devdatatable = (GHashTable *) NULL;

  data->is_grabbed = FALSE;
  data->arrow = FALSE;
  data->rectify = FALSE;
  data->roundify = FALSE;
  data->old_paint_type = ANNOTATE_PEN;

  data->is_cursor_hidden = TRUE;

  data->debug = debug;
  
  /* Initialize the pen context. */
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN);
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER);
  data->cur_context = data->default_pen;
  
  setup_input_devices (data);
  allocate_invisible_cursor (&data->invisible_cursor);
  
  create_savepoint_dir ();

  if (iwb_file)
    {
      data->savepoint_list = load_iwb (iwb_file);
    }

  setup_app (parent);

  return 0;
}


