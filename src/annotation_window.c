/*
 * Ardesia-- a program for painting on the screen
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
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
annotate_paint_context_new (AnnotatePaintType type)
{
  AnnotatePaintContext *context = (AnnotatePaintContext *) NULL;
  gchar* color = g_strdup_printf ("FF0000FF");

  context = g_malloc ((gsize) sizeof (AnnotatePaintContext));
  context->fg_color = color;
  context->type = type;

  return context;
}


/* Calculate the direction in radiant. */
static gdouble
annotate_get_arrow_direction ()
{
  /* Precondition: the list must be not null and the length might be greater than two. */
  AnnotatePoint *point = (AnnotatePoint *) NULL;
  AnnotatePoint *old_point = (AnnotatePoint *) NULL;
  gdouble delta = 2.0;
  gdouble ret = 0.0;
  GSList *out_ptr = data->coord_list;
  gdouble tollerance = data->thickness * delta;

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


/* Colour selector;if eraser than select the transparent colour else allocate the right colour. */
static void
select_color ()
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


/* Configure pen option for cairo context. */
static void 
configure_pen_options ()
{
  if (data->annotation_cairo_context)
    {
      cairo_set_line_cap (data->annotation_cairo_context, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join (data->annotation_cairo_context, CAIRO_LINE_JOIN_ROUND);
      cairo_set_line_width (data->annotation_cairo_context, annotate_get_thickness ());
    }

  select_color ();
}


#ifdef _WIN32


/* Acquire the grab pointer. */
static void
annotate_acquire_pointer_grab ()
{
  grab_pointer (data->annotation_window, ANNOTATE_MOUSE_EVENTS);
}


/* Release the grab pointer. */
static void
annotate_release_pointer_grab ()
{
  ungrab_pointer (gdk_display_get_default ());
}

#endif


/* Update the cursor icon. */
static void
update_cursor ()
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
disallocate_cursor ()
{
  if (data->cursor)
    {
      gdk_cursor_unref (data->cursor);
      data->cursor = (GdkCursor *) NULL;
    }
}


/* Take the input mouse focus. */
static void
annotate_acquire_input_grab ()
{

#ifdef _WIN32
  annotate_acquire_pointer_grab ();
#endif


#ifndef _WIN32
  /* 
   * MACOSX
   * in mac this will do nothing 
   */
  gtk_widget_input_shape_combine_mask (data->annotation_window, NULL, 0, 0);
#endif

}


/* Destroy cairo context. */
static void
destroy_cairo ()
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
annotate_draw_ellipse (gdouble x,
		       gdouble y,
		       gdouble width,
		       gdouble height,
		       gdouble pressure)
{
  if (data->debug)
    {
      g_printerr ("Draw ellipse\n");
    }

  annotate_modify_color (data, pressure);
  cairo_save (data->annotation_cairo_context);

  /* The ellipse is done as a 360 degree arc translated. */
  cairo_translate (data->annotation_cairo_context, x + width / 2., y + height / 2.);
  cairo_scale (data->annotation_cairo_context, width / 2., height / 2.);
  cairo_arc (data->annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore (data->annotation_cairo_context);
}


/* Return if it is a closed path. */
static gboolean
is_a_closed_path (GSList* list)
{
  /* Check if it is a closed path. */
  guint lenght = g_slist_length (list);
  AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (list, 0);
  AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (list, lenght-1);
  gint distance = (gint) get_distance (first_point->x, first_point->y, last_point->x, last_point->y);
  if ( distance == 0)
    {
      return TRUE;
    }
  printf("Distance %d\n", distance);
  return FALSE;
}


/* Draw the point list;the cairo path is not forgotten */
static void 
annotate_draw_point_list (GSList* list)
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
	      annotate_draw_point (point->x, point->y, point->pressure);
	      break;
	    }

	  annotate_modify_color (data, point->pressure);

	  /* Draw line between the two points. */
	  annotate_draw_line (point->x, point->y, FALSE);
	}
    }
}


/* Draw a curve using a cubic bezier splines passing to the list's coordinate. */
static void
annotate_draw_curve (GSList *list)
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
	      annotate_draw_point (first_point->x, first_point->y, first_point->pressure);
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
		      annotate_draw_line (second_point->x, second_point->y, FALSE);
		      return;
		    }

		  annotate_modify_color (data, second_point->pressure);

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
rectify (gboolean closed_path)
{
  gdouble tollerance = data->thickness;
  GSList *broken_list = broken (data->coord_list, closed_path, TRUE, tollerance);

  if (data->debug)
    {
      g_printerr ("rectify\n");
    }

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  annotate_draw_point_list (broken_list);
  
  annotate_coord_list_free ();
  data->coord_list = broken_list;
  
}


/* Roundify the line. */
static void
roundify (gboolean closed_path)
{
  gdouble tollerance = data->thickness;
  gint lenght = g_slist_length (data->coord_list);

  /* Build the meaningful point list with the standard deviation algorithm. */
  GSList *meaningful_point_list = (GSList *) NULL;

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  if ( lenght <= 3)
    {
      /* Draw the point line as is and jump the rounding. */
      annotate_draw_point_list (data->coord_list);
      return;
    }

  meaningful_point_list = build_meaningful_point_list (data->coord_list, closed_path, tollerance);
  if ((closed_path) && (is_similar_to_an_ellipse (meaningful_point_list, tollerance)))
    {
      AnnotatePoint *point1 = (AnnotatePoint *) NULL;
      AnnotatePoint *point3 = (AnnotatePoint *) NULL;

      GSList *rect_list = build_outbounded_rectangle (meaningful_point_list);
      point1 = (AnnotatePoint *) g_slist_nth_data (rect_list, 0);
      point3 = (AnnotatePoint *) g_slist_nth_data (rect_list, 2);
      annotate_draw_ellipse (point1->x, point1->y, point3->x-point1->x, point3->y-point1->y, point1->pressure);
      g_slist_foreach (rect_list, (GFunc)g_free, NULL);
      g_slist_free (rect_list);
    }
  else
    {
      /* It is not an ellipse; I use bezier to spline the path. */
      GSList *splined_list = spline (meaningful_point_list);
      annotate_draw_curve (splined_list);

      annotate_coord_list_free ();
      data->coord_list = splined_list;

    }

  g_slist_foreach (meaningful_point_list, (GFunc) g_free, (gpointer) NULL);
  g_slist_free (meaningful_point_list);
}


/* Is the device the "Eraser"-Type device. */
static gboolean
is_eraser (GdkDevice *device)
{
  if (strstr (device->name, "raser") ||
      strstr (device->name, "RASER"))
    {
      return TRUE;
    }

  return FALSE;
}


/* Set-up input device. */
static void
setup_input_devices ()
{
  GList *devices = (GList *) NULL;
  GList *d = (GList *) NULL;

  devices = gdk_display_list_devices (gdk_display_get_default ());

  for (d = devices; d; d = d->next)
    {
      GdkDevice *device = (GdkDevice *) d->data;

      /* Guess "Eraser"-Type devices. */
      if (is_eraser (device))
	{
	  gdk_device_set_source (device, GDK_SOURCE_ERASER);
	}

      /* Don’t touch devices with lesser than two axis. */
      if (device->num_axes >= 2)
	{

	  if (device->source != GDK_SOURCE_MOUSE)
	    {
	      g_printerr ("Enabled Device. %p: \"%s\" (Type: %d)\n",
			  device, device->name, device->source);

	      if (!gdk_device_set_mode (device, GDK_MODE_SCREEN))
		{
		  g_warning ("Unable to set the device %s to the screen mode\n", device->name);
		}

	    }

	}
    }
}


/* Create the annotation window. */
static GtkWidget *
create_annotation_window ()
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
setup_app (GtkWidget* parent)
{
  gint width = gdk_screen_width ();
  gint height = gdk_screen_height ();
  cairo_t *shape_cr = (cairo_t *) NULL;

  /* Initialize the pen context. */
  data->default_pen = annotate_paint_context_new (ANNOTATE_PEN);
  data->default_eraser = annotate_paint_context_new (ANNOTATE_ERASER);
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;

  setup_input_devices ();

  /* Create the annotation window. */
  data->annotation_window = create_annotation_window ();

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

  gtk_window_set_transient_for (GTK_WINDOW (data->annotation_window), GTK_WINDOW (parent));

  gtk_widget_set_usize (data->annotation_window, width, height);

  /* Initialize a transparent pixmap with depth 1 to be used as input shape. */
  data->shape = gdk_pixmap_new ((GdkDrawable *) NULL, width, height, 1);

  shape_cr = gdk_cairo_create (data->shape);

  if (cairo_status (shape_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Failed to allocate the shape cairo context");
      annotate_quit ();
      exit (EXIT_FAILURE);
    }

  clear_cairo_context (shape_cr);
  cairo_destroy (shape_cr);

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals (data->annotation_window_gtk_builder, (gpointer) data);

  gtk_widget_show_all (data->annotation_window);

  /* This put the window in fullscreen generating an exposure. */
  gtk_window_fullscreen (GTK_WINDOW (data->annotation_window));


#ifdef _WIN32
  /* @TODO Use RGBA colormap and avoid to use the layered window. */
  /* I use a layered window that use the black as transparent colour. */
  setLayeredGdkWindowAttributes (gtk_widget_get_window(data->annotation_window),
				 RGB (0,0,0),
				 0,
				 LWA_COLORKEY );	
#endif
}


/* Create the directory where put the save-point files. */
static void
create_savepoint_dir ()
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
delete_savepoint (AnnotateSavepoint *savepoint)
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
void static
annotate_redolist_free ()
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
delete_ardesia_tmp_dir()
{
  gchar *ardesia_tmp_dir = g_build_filename (g_get_tmp_dir (), PACKAGE_NAME, (gchar *) 0);
  rmdir_recursive (ardesia_tmp_dir);
  g_free (ardesia_tmp_dir);
}


/* Draw an arrow starting from the point 
 * whith the width and the direction in radiant
 */
static void 
draw_arrow_in_point(AnnotatePoint *point,
		    gdouble width,
		    gdouble direction)
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


/*
 * Add a save point for the undo/redo;
 * this code must be called at the end of each painting action.
 */
void
annotate_add_savepoint ()
{
  AnnotateSavepoint *savepoint = g_malloc ((gsize) sizeof (AnnotateSavepoint));
  cairo_surface_t *saved_surface = (cairo_surface_t *) NULL;
  cairo_surface_t *source_surface = (cairo_surface_t *) NULL;
  cairo_t *cr = (cairo_t *) NULL;

  guint savepoint_index = g_slist_length (data->savepoint_list) + 1;
  
  savepoint->filename = g_strdup_printf ("%s%s%s_%d_vellum.png",
					 data->savepoint_dir,
					 G_DIR_SEPARATOR_S,
					 PACKAGE_NAME,
					 savepoint_index);

  /* The story about the future is deleted. */
  annotate_redolist_free ();

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
initialize_annotation_cairo_context(AnnotateData *data)
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
	  annotate_reset_cairo ();
	  clear_cairo_context (data->annotation_cairo_context);
	}
#ifndef _WIN32
      gtk_window_set_opacity(GTK_WINDOW(data->annotation_window), 1.0);
#endif 	
      annotate_acquire_grab ();
    }
}


/* Draw the last save point on the window restoring the surface. */
void
annotate_restore_surface ()
{
  if (data->annotation_cairo_context)
    {
      guint i = data->current_save_index;
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
get_annotation_window ()
{
  return data->annotation_window;
}


/* Set colour. */
void
annotate_set_color (gchar *color)
{
  data->cur_context->fg_color = color;
}


/* Set rectifier. */
void
annotate_set_rectifier (gboolean rectify)
{
  data->rectify = rectify;
}


/* Set rounder. */
void
annotate_set_rounder (gboolean roundify)
{
  data->roundify = roundify;
}


/* Set arrow. */
void
annotate_set_arrow (gboolean arrow)
{
  data->arrow = arrow;
}


/* Set the line thickness. */
void
annotate_set_thickness (gdouble thickness)
{
  data->thickness = thickness;
}


/* Get the line thickness. */
gdouble
annotate_get_thickness ()
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
annotate_coord_list_prepend (gdouble x,
			     gdouble y,
			     gdouble width,
			     gdouble pressure)
{
  AnnotatePoint *point = g_malloc ((gsize) sizeof (AnnotatePoint));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  data->coord_list = g_slist_prepend (data->coord_list, point);
}


/* Free the list of the painted point. */
void
annotate_coord_list_free ()
{
  if (data->coord_list)
    {
      g_slist_foreach (data->coord_list, (GFunc) g_free, (gpointer) NULL);
      g_slist_free (data->coord_list);
      data->coord_list = (GSList *) NULL;
    }
}


/* Modify colour according to the pressure. */
void
annotate_modify_color (AnnotateData *data,
		       gdouble pressure)
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
  if ( (!data->annotation_cairo_context)|| (!data->cur_context->fg_color))
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

  if (data->coord_list)
    {
      AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (data->coord_list, 0);
      old_pressure = last_point->pressure;
    }

  corrective = (1- ( 3 * pressure + old_pressure)/4) * contrast;
  cairo_set_source_rgba (data->annotation_cairo_context,
			 (r + corrective)/255,
			 (g + corrective)/255,
			 (b+corrective)/255,
			 (gdouble) a/255);
}


/* Set a new cairo path with the new options. */
void
annotate_reset_cairo ()
{
  if (data->annotation_cairo_context)
    {
      cairo_new_path (data->annotation_cairo_context);
    }

  configure_pen_options ();
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

  annotate_reset_cairo ();
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
annotate_select_pen ()
{
  if (data->debug)
    {
      g_printerr ("The pen with colour %s has been selected\n", data->cur_context->fg_color);
    }

  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;

  disallocate_cursor ();

  set_pen_cursor (&data->cursor, data->thickness, data->cur_context->fg_color, data->arrow);

  update_cursor ();
}


/* Select the default eraser tool. */
void
annotate_select_eraser ()
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
annotate_hide_cursor ()
{
  gdk_window_set_cursor (gtk_widget_get_window (data->annotation_window), data->invisible_cursor);
  data->is_cursor_hidden = TRUE;
}


/* acquire the grab. */
void
annotate_acquire_grab ()
{
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
annotate_draw_line (gdouble x2,
		    gdouble y2,
		    gboolean stroke)
{
  if (!stroke)
    {
      cairo_line_to (data->annotation_cairo_context, x2, y2);
    }
  else
    {
      cairo_line_to (data->annotation_cairo_context, x2, y2);
      cairo_stroke (data->annotation_cairo_context);
      cairo_move_to (data->annotation_cairo_context, x2, y2);
    }
}


/* Draw an arrow using some polygons. */
void
annotate_draw_arrow (gdouble distance)
{
  gdouble direction = 0;
  gdouble pen_width = data->thickness;
  gdouble arrow_minimum_size = pen_width * 2;

  AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (data->coord_list, 0);

  if (distance < arrow_minimum_size)
    {
      return;
    }

  if (data->debug)
    {
      g_printerr ("Draw arrow: ");
    }

  if (g_slist_length (data->coord_list) < 2)
    {
      /* If it has length lesser then two then is a point and it has no sense draw the arrow. */
      return;
    }

  /* Postcondition length >= 2 */
  direction = annotate_get_arrow_direction ();

  if (data->debug)
    {
      g_printerr ("Arrow direction %f\n", direction/M_PI*180);
    }

  draw_arrow_in_point(point, pen_width, direction);
}


/* Draw a point in x,y respecting the context. */
void
annotate_draw_point (gdouble x,
		     gdouble y,
		     gdouble pressure)
{
  /* Modify a little bit the colour depending on pressure. */
  annotate_modify_color (data, pressure);
  cairo_move_to (data->annotation_cairo_context, x, y);
  cairo_line_to (data->annotation_cairo_context, x, y);
}


/* Call the geometric shape recognizer. */
void
annotate_shape_recognize (gboolean closed_path)
{
  if (data->rectify)
    {
      rectify (closed_path);
    }
  else if (data->roundify)
    {
      roundify (closed_path);
    }
}


/* Select eraser, pen or other tool for tablet. */
void
annotate_select_tool (AnnotateData *data, GdkDevice *device, guint state)
{
  if (device)
    {

      if (device->source == GDK_SOURCE_ERASER)
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
      data->cur_context = data->default_pen;
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
annotate_quit ()
{
  export_iwb (get_iwb_filename ());
  if (data)
    {

      if (data->shape)
	{
	  g_object_unref (data->shape);
	  data->shape = (GdkPixmap *) NULL;
	}
	
      /* Destroy cairo object. */
      destroy_cairo ();

      /* Destroy cursors. */
      disallocate_cursor ();
      cursors_main_quit ();

      if (data->invisible_cursor)
	{
	  gdk_cursor_unref (data->invisible_cursor);
	  data->invisible_cursor = (GdkCursor *) NULL;
	}

      /* Free all. */
      if (data->annotation_window)
	{
	  gtk_widget_destroy (data->annotation_window);
	  data->annotation_window = (GtkWidget *) NULL;
	}

      annotate_coord_list_free ();
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
annotate_release_input_grab ()
{
  gdk_window_set_cursor (gtk_widget_get_window (data->annotation_window), (GdkCursor *) NULL);
#ifndef _WIN32
  /*
   * @TODO implement correctly gdk_window_input_shape_combine_mask
   * in the quartz gdkwindow or use an equivalent native function;
   * the current implementation in macosx this does not do nothing.
   */
  /*
   * This allows the mouse event to be passed below the transparent annotation;
   * at the moment this call works only on Linux
   */
  gdk_window_input_shape_combine_mask (gtk_widget_get_window (data->annotation_window), data->shape, 0, 0);
#else
  /*
   * @TODO WIN32 implement correctly gdk_window_input_shape_combine_mask
   * in the win32 gdkwindow or use an equivalent native function.
   * Now in the gtk implementation the gdk_window_input_shape_combine_mask
   * call the gdk_window_shape_combine_mask that is not the desired behaviour.
   *
   */
  annotate_release_pointer_grab ();
#endif
}


/* Release the pointer pointer. */
void
annotate_release_grab ()
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


/* Fill the last shape. */
void annotate_fill ()
{
  if (data->debug)
    {
      g_printerr ("Fill\n");
    }

  if (data->coord_list)
    {

      /* if is not a closed path I prevent to fill it */
      if (!is_a_closed_path (data->coord_list))
	{
	  if (data->debug)
	    {
	      g_printerr ("Fill is not done; no closed path has been detected\n");
	    }
	  return;
	}

      if ( !(data->roundify) && (!(data->rectify)))
	{
	  annotate_draw_point_list (data->coord_list);
	}

      cairo_close_path (data->annotation_cairo_context);
      select_color ();
      cairo_fill (data->annotation_cairo_context);
      cairo_stroke (data->annotation_cairo_context);

      if (data->debug)
	{
	  g_printerr ("Fill\n");
	}

      annotate_add_savepoint ();
    }
}


/* Undo reverting to the last save point. */
void
annotate_undo ()
{
  if (data->savepoint_list)
    {
      if (data->current_save_index != g_slist_length (data->savepoint_list)-1)
	{

	  if (data->debug)
	    {
	      g_printerr ("Undo\n");
	    }

	  data->current_save_index = data->current_save_index + 1;
	  annotate_restore_surface ();
	}
    }
}


/* Redo to the last save point. */
void annotate_redo ()
{
  if (data->savepoint_list)
    {
      if (data->current_save_index != 0)
	{

	  if (data->debug)
	    {
	      g_printerr ("Redo\n");
	    }

	  data->current_save_index = data->current_save_index - 1;
	  annotate_restore_surface ();
	}
    }
}


/* Clear the annotations windows. */
void annotate_clear_screen ()
{
  if (data->debug)
    {
      g_printerr ("Clear screen\n");
    }

  annotate_reset_cairo ();
  clear_cairo_context (data->annotation_cairo_context);
  annotate_add_savepoint ();
}


/* Initialize the annotation. */
gint 
annotate_init (GtkWidget *parent,
	       gchar *iwb_file,
	       gboolean debug)
{
  cursors_main ();
  data = g_malloc ((gsize) sizeof (AnnotateData));

  /* Initialize the data structure. */
  data->annotation_cairo_context = (cairo_t *) NULL;
  data->coord_list = (GSList *) NULL;
  data->savepoint_list = (GSList *) NULL;
  data->current_save_index = 0;
  data->cursor = (GdkCursor *) NULL;

  data->is_grabbed = FALSE;
  data->arrow = FALSE;
  data->rectify = FALSE;
  data->roundify = FALSE;

  data->is_cursor_hidden = TRUE;

  data->debug = debug;
  
  allocate_invisible_cursor (&data->invisible_cursor);
  
  create_savepoint_dir ();

  if (iwb_file)
    {
      data->savepoint_list = load_iwb (iwb_file);
    }

  setup_app (parent);

  return 0;
}

it
