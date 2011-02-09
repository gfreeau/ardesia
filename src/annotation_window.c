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


#include <annotation_window.h>
#include <annotation_window_callbacks.h>
#include <utils.h>
#include <broken.h>
#include <background_window.h>
#include <bezier_spline.h>
#include <iwb_loader.h>
#include <iwb_saver.h>


#ifdef _WIN32
#  include <windows_utils.h>
#endif


/* Internal data for the annotation window. */
static AnnotateData* data;


/* Create a new paint context. */
static AnnotatePaintContext* annotate_paint_context_new(AnnotatePaintType type)
{
  AnnotatePaintContext *context = g_malloc ((gsize) sizeof(AnnotatePaintContext));
  context->type = type;
  context->fg_color = g_strdup_printf("FF0000FF");
  return context;
}


/* Calculate the direction in radiants. */
static gdouble annotate_get_arrow_direction()
{
  /* The list must be not null and the lenght might be greater than two. */
  GSList *out_ptr = data->coord_list;   
  gdouble delta = 2.0;

  gdouble tollerance = data->thickness * delta;
  gdouble ret;  

  AnnotateStrokeCoordinate* point = NULL;
  AnnotateStrokeCoordinate* old_point = NULL;
  if (g_slist_length(out_ptr) >= 3)
    {
      /* Extract the relevant point with the standard deviation. */
      GSList *relevantpoint_list = extract_relevant_points(out_ptr, FALSE, tollerance);
      old_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (relevantpoint_list, 1);
      point = (AnnotateStrokeCoordinate*) g_slist_nth_data (relevantpoint_list, 0);
      /* Give the direction using the last two point. */
      ret = atan2 (point->y-old_point->y, point->x-old_point->x);
      /* Free the relevant point list. */
      g_slist_foreach(relevantpoint_list, (GFunc)g_free, NULL);
      g_slist_free(relevantpoint_list);
    }  
  else
    {
      old_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (out_ptr, 1);
      point = (AnnotateStrokeCoordinate*) g_slist_nth_data (out_ptr, 0);
      /* Calculate the tan beetween the last two point directly. */
      ret = atan2 (point->y-old_point->y, point->x-old_point->x);
    }
  return ret;
}


/* Color selector; if eraser than select the transparent color else alloc the right color. */ 
static void select_color()
{
  if (!data->annotation_cairo_context)
    {
      return;
    }
  if (data->cur_context)
    {
      if (!(data->cur_context->type == ANNOTATE_ERASER))
        {
          /* Pen or arrow tool. */
          if (data->debug)
	    { 
	      g_printerr("Select color %s\n", data->cur_context->fg_color);
	    }
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
          if (data->cur_context->fg_color)
            {
              cairo_set_source_color_from_string(data->annotation_cairo_context, 
						 data->cur_context->fg_color);
            }
        }
      else
        {
          /* It is the eraser tool. */
          if (data->debug)
	    { 
	      g_printerr("The eraser tool has been selected\n");
	    }
          cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_CLEAR);
        }
    }
}


/* Configure pen option for cairo context. */
static void configure_pen_options()
{
  if (data->annotation_cairo_context)
    {
      cairo_set_line_cap (data->annotation_cairo_context, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_ROUND); 
      cairo_set_line_width(data->annotation_cairo_context, annotate_get_thickness());
    }
  select_color();  
}


#ifdef _WIN32

  
/* Acquire the grab pointer. */
static void annotate_acquire_pointer_grab()
{
  grab_pointer(data->annotation_window, ANNOTATE_MOUSE_EVENTS);
}


/* Release the grab pointer. */
static void annotate_release_pointer_grab()
{
  ungrab_pointer(gdk_display_get_default(), data->annotation_window);
}

#endif


/* Update the cursor icon. */
static void update_cursor()
{
  if (!data->annotation_window)
    {
      return;
    }

#ifdef _WIN32
  annotate_release_pointer_grab();
#endif

  gdk_window_set_cursor (data->annotation_window->window, data->cursor);

#ifdef _WIN32
  annotate_acquire_pointer_grab();
#endif
}


/* Create pixmap and mask for the invisible cursor; 
 * this is used to hide the cursor. 
 */
static void get_invisible_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  cairo_t *invisible_cr = NULL;
  cairo_t *invisible_shape_cr = NULL;
  
  *pixmap = gdk_pixmap_new (NULL, size, size, 1);
  *mask =  gdk_pixmap_new (NULL, size, size, 1); 
  invisible_cr = gdk_cairo_create(*pixmap);
  
  if (cairo_status(invisible_cr) != CAIRO_STATUS_SUCCESS)
    {
      if (data->debug)
	{ 
	  g_printerr ("Failed to allocate the cairo context to hide the cursor"); 
	  annotate_quit(); 
	  exit(EXIT_FAILURE);
	}
    }
  cairo_set_source_rgb(invisible_cr, 1, 1, 1);
  cairo_paint(invisible_cr);
  cairo_stroke(invisible_cr);
  cairo_destroy(invisible_cr);

  invisible_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(invisible_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
      if (data->debug)
	{ 
	  g_printerr ("Failed to allocate the cairo context for the surface to be restored\n"); 
	  annotate_quit(); 
	  exit(EXIT_FAILURE);
	}
    }
  clear_cairo_context(invisible_shape_cr);
  cairo_stroke(invisible_shape_cr);
  cairo_destroy(invisible_shape_cr); 
}


/* Allocate a invisible cursor that can be used to hide the cursor icon. */
static void allocate_invisible_cursor()
{
  GdkPixmap *pixmap, *mask;
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor(WHITE);
  
  get_invisible_pixmaps(1, &pixmap, &mask);
  
  data->invisible_cursor = gdk_cursor_new_from_pixmap (pixmap, mask,
						       foreground_color_p,
						       background_color_p, 
						       0, 0);
  g_object_unref(pixmap);
  g_object_unref(mask);
  g_free(foreground_color_p);
  g_free(background_color_p);			
}


/* Create pixmap and mask for the eraser cursor. */
static void get_eraser_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint circle_width = 2; 
  cairo_t *eraser_cr = NULL;
  cairo_t *eraser_shape_cr = NULL;
  
  *pixmap = gdk_pixmap_new(NULL, size, size, 1);
  *mask =  gdk_pixmap_new(NULL, size, size, 1);
 
  eraser_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(eraser_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr("Failed to allocate the eraser cursor cairo context"); 
      annotate_quit();
      exit(EXIT_FAILURE);
    }

  cairo_paint(eraser_cr);
  cairo_stroke(eraser_cr);
  cairo_destroy(eraser_cr);

  eraser_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(eraser_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr("Failed to allocate the eraser shape cursor cairo context"); 
      annotate_quit(); 
      exit(EXIT_FAILURE);
    }

  /* Paint the eraser circle icon by code. */
  clear_cairo_context(eraser_shape_cr);
  cairo_set_operator(eraser_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(eraser_shape_cr, circle_width);
  cairo_set_source_rgb(eraser_shape_cr, 1, 1, 1);
  cairo_arc(eraser_shape_cr, size/2, size/2, (size/2)-circle_width, 0, 2 * M_PI);
  cairo_stroke(eraser_shape_cr);
  cairo_destroy(eraser_shape_cr);
}


/* Create pixmap and mask for the pen cursor. */
static void get_pen_pixmaps(gint size, GdkPixmap** pixmap, GdkPixmap** mask)
{
  gint side_lenght = (size*3) + data->thickness;
  gint circle_width = 2; 
  cairo_t *pen_cr = NULL;
  cairo_t *pen_shape_cr = NULL;
  
  *pixmap = gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);
  *mask =  gdk_pixmap_new (NULL, side_lenght, side_lenght, 1);

  pen_cr = gdk_cairo_create(*pixmap);
  if (cairo_status(pen_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Failed to allocate the pen cursor cairo context"); 
      annotate_quit(); 
      exit(EXIT_FAILURE);
    }

  cairo_set_operator(pen_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgb(pen_cr, 1, 1, 1);
  cairo_paint(pen_cr);
  cairo_stroke(pen_cr);
  cairo_destroy(pen_cr);

  pen_shape_cr = gdk_cairo_create(*mask);
  if (cairo_status(pen_shape_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Failed to allocate the pen shape cursor cairo context"); 
      annotate_quit(); 
      exit(EXIT_FAILURE);
    }

  /* Draw the pen cursor by code. */
  clear_cairo_context(pen_shape_cr);
  cairo_set_operator(pen_shape_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(pen_shape_cr, circle_width);
  cairo_set_source_rgb(pen_shape_cr, 1, 1, 1);
  cairo_arc(pen_shape_cr, 5* size/2 + data->thickness/2, size/2, 
            (size/2)-circle_width, M_PI * 5/4, M_PI/4);
  cairo_arc(pen_shape_cr, size/2 + data->thickness/2, 5 * size/2,
            (size/2)-circle_width, M_PI/4, M_PI * 5/4); 
  cairo_fill(pen_shape_cr);
  cairo_arc(pen_shape_cr, size/2 + data->thickness/2 , 5 * size/2,
            data->thickness/2, 0, 2 * M_PI);
  cairo_stroke(pen_shape_cr);
  cairo_destroy(pen_shape_cr);
}


/* Disallocate cursor. */
static void disallocate_cursor()
{
  if (data->cursor)
    {
      gdk_cursor_unref(data->cursor);
      data->cursor = NULL;
    }
}


/* Set the cursor patching the xpm with the selected color. */
static void annotate_set_pen_cursor()
{
  gint size=12;
  GdkPixmap *pixmap, *mask;
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor(data->cur_context->fg_color); 
  gint thickness = data->thickness;

  disallocate_cursor();

  get_pen_pixmaps(size, &pixmap, &mask); 

  if (data->debug)
    {
      g_printerr("The color %s has been selected\n", data->cur_context->fg_color);
    }  

  data->cursor = gdk_cursor_new_from_pixmap (pixmap, mask, 
                                             foreground_color_p, 
                                             background_color_p,
                                             size/2 + thickness/2, 5* size/2);
  
  g_object_unref(pixmap);
  g_object_unref(mask);
  g_free(foreground_color_p);
  g_free(background_color_p); 
  update_cursor();
}


/* Set the eraser cursor. */
static void annotate_set_eraser_cursor()
{   
  gint size = annotate_get_thickness();
  GdkPixmap *pixmap, *mask;
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor(RED);
  
  disallocate_cursor();

  get_eraser_pixmaps(size, &pixmap, &mask); 
  
 
  data->cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                            foreground_color_p, 
                                            background_color_p, 
                                            size/2, size/2);
 
  g_object_unref(pixmap);
  g_object_unref(mask);
  g_free(foreground_color_p);
  g_free(background_color_p);
  update_cursor();
}


/* Take the input mouse focus. */
static void annotate_acquire_input_grab()
{

#ifdef _WIN32
  annotate_acquire_pointer_grab();
#endif
 
 
#ifndef _WIN32
  /* 
   * MACOSX
   * in mac this will do nothing 
   */
  gtk_widget_input_shape_combine_mask(data->annotation_window, NULL, 0, 0); 
#endif

}


/* Destroy cairo context. */
static void destroy_cairo()
{
  gint refcount = cairo_get_reference_count(data->annotation_cairo_context);
  
  gint i = 0;
  for  (i=0; i<refcount; i++)
    {
      cairo_destroy(data->annotation_cairo_context);
    }
}


/* This an ellipse taking the top left edge coordinates the width and the height 
 * of the bounded rectangle. 
 */
static void annotate_draw_ellipse(gint x, gint y, gint width, gint height)
{
  if (data->debug)
    {
      g_printerr("Draw ellipse\n");
    }

  cairo_save(data->annotation_cairo_context);

  /* The ellipse is done as a 360 degree arc translated. */
  cairo_translate(data->annotation_cairo_context, x + width / 2., y + height / 2.);
  cairo_scale(data->annotation_cairo_context, width / 2., height / 2.);
  cairo_arc(data->annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore(data->annotation_cairo_context);
}


/* Draw the point list. */
static void annotate_draw_point_list(GSList* list)
{

  if (list)
    {
      while (list)
        {
          AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)list->data;
          gdouble cur_x = out_point->x; 
          gdouble cur_y = out_point->y;

          annotate_modify_color(data, out_point->pressure); 
	  /* Draw line beetween the two points. */
	  annotate_draw_line(cur_x, cur_y, FALSE);
	  list = list->next;   
	}
    }
}


/* Draw a curve using a cubic bezier splines passing to the list's coordinate. */
static void annotate_draw_curve(GSList* list)
{
  gint lenght = g_slist_length(list);

  if (list)
    {
      gint i = 0;  
      for (i=0; i<lenght; i=i+3)
        {

	  AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i);
	  AnnotateStrokeCoordinate* second_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i+1);
	  AnnotateStrokeCoordinate* third_point = (AnnotateStrokeCoordinate*) g_slist_nth_data(list, i+2);

	  if (!second_point)
	    {
	      return;
	    }

	  if (!third_point)
	    {
	      /* draw line from first to second point */
	      annotate_draw_line(second_point->x, second_point->y, FALSE);
	      return;
	    } 

	  cairo_curve_to(data->annotation_cairo_context,
			 first_point->x, first_point->y,
			 second_point->x, second_point->y,
			 third_point->x, third_point->y);     

	}
    }
}


/* Rectify the line. */
static void rectify(gboolean closed_path)
{
  gint tollerance = data->thickness;
  GSList *out_ptr = broken(data->coord_list, closed_path, TRUE, tollerance);

  if (data->debug)
    {
      g_printerr("rectify\n");
    }

  /* Restore the surface withgout the last path handwritten. */
  annotate_restore_surface();

  annotate_coord_list_free();
  data->coord_list = out_ptr;

  annotate_draw_point_list(out_ptr);     
  if (closed_path)
    {
      cairo_close_path(data->annotation_cairo_context);   
    } 
}


/* Roundify the line. */
static void roundify(gboolean closed_path)
{
  gint tollerance = data->thickness * 2;
  GSList *out_ptr = extract_relevant_points(data->coord_list, closed_path, tollerance);  
  gint lenght = g_slist_length(out_ptr);
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coord_list,
										  lenght/2);

  /* Restore the surface withgout the last path handwritten. */
  annotate_restore_surface();
     
  annotate_coord_list_free();
  data->coord_list = out_ptr;
  annotate_modify_color(data, point->pressure); 
 
  if (lenght == 1)
    {
      /* It is a point. */ 
      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)out_ptr->data;
      annotate_draw_point(out_point->x, out_point->y, out_point->pressure);
      return;
    }
  if (lenght <= 3)
    {
      /* Draw the point line as is and jump the rounding. */
      annotate_draw_point_list(out_ptr);
      return; 
    }

  if (closed_path)
    {
      /* It could be an ellipse or a closed curve path. */  
      GSList* list_out_n = extract_outbounded_rectangle(out_ptr);

      AnnotateStrokeCoordinate* out_point = (AnnotateStrokeCoordinate*)list_out_n->data;
      gdouble lastx = out_point->x; 
      gdouble lasty = out_point->y;
      AnnotateStrokeCoordinate* point3 = (AnnotateStrokeCoordinate*) g_slist_nth_data (list_out_n, 2);

      /* 
       * If in one point the sum of the distance by focus F1 and F2 differer more than 
       * the tollerance value the curve line will not be considered an eclipse. 
       */
      gdouble tollerance = (fabs(point3->x-lastx)+fabs(point3->y-lasty)) /4;

      if (is_similar_to_an_ellipse(out_ptr, list_out_n, tollerance))
	{
	  annotate_draw_ellipse(lastx, lasty, point3->x-lastx, point3->y-lasty);          
	}
      else
	{
	  /* It is a closed path but it is not an eclipse; I use bezier to spline the path. */
	  GSList* splined_list = spline(out_ptr);
	  annotate_coord_list_free();
	  data->coord_list = splined_list;
	  annotate_draw_curve(splined_list);
	}

      /* Disallocate the outbounded rectangle. */
      g_slist_foreach(list_out_n, (GFunc)g_free, NULL);
      g_slist_free(list_out_n);
    }   
  else
    {
      /* It's a not closed path than I use bezier to spline the path. */
      GSList* splined_list = spline(out_ptr);
      annotate_coord_list_free();
      data->coord_list = splined_list;
      annotate_draw_curve(splined_list);
    }
}


/* Is the device the "Eraser"-Type device. */
static gboolean is_eraser(GdkDevice *device)
{
  if (strstr(device->name, "raser") || 
      strstr(device->name, "RASER"))
    {
      return TRUE;
    }
  return FALSE;
}


/* Setup input device. */
static void setup_input_devices()
{
  GList     *devices, *d;

  devices = gdk_display_list_devices(gdk_display_get_default());
  for (d = devices; d; d = d->next)
    {
      GdkDevice *device = (GdkDevice *) d->data;

      /* Guess "Eraser"-Type devices. */
      if (is_eraser(device))
        {
	  gdk_device_set_source(device, GDK_SOURCE_ERASER);
        }

      /* Dont touch devices with lesser than two axis. */
      if (device->num_axes >= 2)
        {
	  if (device->source != GDK_SOURCE_MOUSE)
	    { 
              g_printerr("Enabled Device. %p: \"%s\" (Type: %d)\n",
			 device, device->name, device->source);
              gdk_device_set_mode(device, GDK_MODE_SCREEN);
	    }
        }
    }
}


/* Create the annotation window. */
static GtkWidget* create_annotation_window()
{
  GtkWidget* widget = NULL;
  GError* error = NULL;

  /* Initialize the main window. */
  data->annotation_window_gtk_builder = gtk_builder_new();

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file(data->annotation_window_gtk_builder, ANNOTATION_UI_FILE, &error);

  if (error)
    {
      g_warning ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      return widget;
    }  
 
  widget = GTK_WIDGET(gtk_builder_get_object(data->annotation_window_gtk_builder, 
					     "annotationWindow")); 
   
  return widget;
}


/* Setup the application. */
static void setup_app(GtkWidget* parent)
{
  /* Initialize the pen context. */
  data->default_pen = annotate_paint_context_new(ANNOTATE_PEN);
  data->default_eraser = annotate_paint_context_new(ANNOTATE_ERASER);
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;
  
  setup_input_devices();
  
  /* Create the annotation window. */
  data->annotation_window = create_annotation_window();

  /* Put the opacity to 0 to avoid the initial flickering. */
  gtk_window_set_opacity(GTK_WINDOW(data->annotation_window), 0);
  
  if (data->annotation_window == NULL)
    {
      g_warning("Failed to create the annotation window");
      return;
    }
   
  gtk_window_set_transient_for(GTK_WINDOW(data->annotation_window), GTK_WINDOW(parent));
  
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();

  gtk_widget_set_usize(data->annotation_window, width, height);

   
  /* Initialize a transparent pixmap with depth 1 to be used as input shape. */
  data->shape = gdk_pixmap_new(NULL, width, height, 1); 

  cairo_t *shape_cr = gdk_cairo_create(data->shape);
  if (cairo_status(shape_cr) != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Failed to allocate the shape cairo context");
      annotate_quit(); 
      exit(EXIT_FAILURE);
    }

  clear_cairo_context(shape_cr);  
  cairo_destroy(shape_cr);

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals(data->annotation_window_gtk_builder, (gpointer) data); 

  gtk_widget_show_all(data->annotation_window);

  /* This put the window in fullscreen generating an exposure. */
  gtk_window_fullscreen(GTK_WINDOW(data->annotation_window));
  
#ifdef _WIN32
  /* In the gtk 2.16.6 the gtkbuilder property double-buffered is not parsed from the glade file 
   * and then I set this by hands. 
   */
  gtk_widget_set_double_buffered(data->annotation_window, FALSE); 
  /* @TODO Use RGBA colormap and avoid to use the layered window. */
  /* I use a layered window that use the black as transparent color. */
  setLayeredGdkWindowAttributes(data->annotation_window->window, 
				RGB(0,0,0), 
				0,
				LWA_COLORKEY );	
#endif
}


/* Return if it is a closed path. */
static gboolean is_a_closed_path(GSList* list)
{
  /* Check if it is a closed path. */
  gint lenght = g_slist_length(list);
  AnnotateStrokeCoordinate* first_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, 0);
  AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (list, lenght-1);
  if (get_distance(first_point->x, first_point->y, last_point->x, last_point->y) > 0)
    {
      return TRUE;
    }
  return FALSE;
}


/* Create the directory where put the savepoint files. */
static void create_savepoint_dir()
{
  const gchar* tmpdir = g_get_tmp_dir();
  gchar* images = "images";
  gchar* project_name = get_project_name();

  gchar* ardesia_tmp_dir = g_build_filename(tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar* project_tmp_dir = g_build_filename(ardesia_tmp_dir, project_name, (gchar *) 0);
  if (g_file_test(ardesia_tmp_dir, G_FILE_TEST_IS_DIR))
    { 
      /* The folder already exist; I delete it. */
      rmdir_recursive(ardesia_tmp_dir);
    }

  data->savepoint_dir = g_build_filename(project_tmp_dir, images, (gchar *) 0);
  g_mkdir_with_parents(data->savepoint_dir, 0777);
  g_free(ardesia_tmp_dir);
  g_free(project_tmp_dir);
}


/* Delete the savepoint. */
static void delete_savepoint(AnnotateSavepoint* savepoint)
{
  if (savepoint)
    {
      if (data->debug)
	{ 
	  g_printerr("The savepoint %s has been removed\n", savepoint->filename);
	}
      if (savepoint->filename)
	{
	  g_remove(savepoint->filename);
	  g_free(savepoint->filename);
	}
      data->savepoint_list = g_slist_remove(data->savepoint_list, savepoint);
      g_free(savepoint);
      savepoint = NULL;
    }
}


/* Free the list of the  savepoint for the redo. */
void static annotate_redolist_free()
{
  gint i = data->current_save_index;
  GSList *stop_list = g_slist_nth (data->savepoint_list, i);
  
  while (data->savepoint_list!=stop_list)
    {
      AnnotateSavepoint* savepoint = (AnnotateSavepoint*) g_slist_nth_data (data->savepoint_list, 0);
      delete_savepoint(savepoint);  
    }
}


/* Free the list of all the savepoint. */
static void annotate_savepoint_list_free()
{
  while (data->savepoint_list!=NULL)
    {
      AnnotateSavepoint* savepoint = (AnnotateSavepoint*) g_slist_nth_data (data->savepoint_list, 0);
      delete_savepoint(savepoint);
    }
}


/* 
 * Add a save point for the undo/redo; 
 * this code must be called at the end of each painting action.
 */
void annotate_add_savepoint()
{
  AnnotateSavepoint *savepoint = g_malloc((gsize) sizeof(AnnotateSavepoint));

  gint savepoint_index = g_slist_length(data->savepoint_list) + 1;
  
  savepoint->filename = g_strdup_printf("%s%s%s_%d_vellum.png", 
					data->savepoint_dir, 
					G_DIR_SEPARATOR_S, 
					PACKAGE_NAME,
					savepoint_index);

  /* The story about the future is deleted. */
  annotate_redolist_free();

  /* Add a new savepoint. */
  data->savepoint_list = g_slist_prepend(data->savepoint_list, savepoint);
  data->current_save_index = 0;
   
  /* Load a surface with the data->annotation_cairo_context content and write the file. */
  cairo_surface_t* saved_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
							      gdk_screen_width(), 
							      gdk_screen_height());
 
  cairo_surface_t* source_surface = cairo_get_target(data->annotation_cairo_context);
  cairo_t *cr = cairo_create(saved_surface);
  cairo_set_source_surface(cr, source_surface, 0, 0);
  cairo_paint(cr);
  /* The saved_surface now contains the savepoint image. */  
  

  /*  Will be create a file in the savepoint folder with format PACKAGE_NAME_1.png. */
  cairo_surface_write_to_png(saved_surface, savepoint->filename);
  cairo_surface_destroy(saved_surface); 
  if (data->debug)
    { 
      g_printerr("The save point %s has been stored in file\n", savepoint->filename);
    }
    
  cairo_destroy(cr);
}


/* Draw the last save point on the window restoring the surface. */
void annotate_restore_surface()
{
  if (data->annotation_cairo_context)
    {
      gint i = data->current_save_index;
      AnnotateSavepoint* savepoint = (AnnotateSavepoint*) g_slist_nth_data(data->savepoint_list, i);
      if (!savepoint)
	{ 
	  return;
	}
 
      cairo_new_path(data->annotation_cairo_context);
      cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);

      if (savepoint->filename)
	{
	  if (data->debug)
	    {
	      g_printerr("The savepoint %s has been loaded from file\n", savepoint->filename);
	    }

	  /* Load the file in the annotation surface. */
	  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (savepoint->filename, NULL);
	  if (pixbuf)
	    {
	      gdk_cairo_set_source_pixbuf(data->annotation_cairo_context, pixbuf, 0.0, 0.0);
	      cairo_paint(data->annotation_cairo_context);
	      g_object_unref (pixbuf);
	    }
	}

    }
}


/* Get the annotation window. */
GtkWidget* get_annotation_window()
{
  return data->annotation_window;
}


/* Set color. */
void annotate_set_color(gchar* color)
{
  data->cur_context->fg_color = color;
}


/* Set rectifier. */
void annotate_set_rectifier(gboolean rectify)
{
  data->rectify = rectify;
}


/* Set rounder. */
void annotate_set_rounder(gboolean roundify)
{
  data->roundify = roundify;
}


/* Set arrow. */
void annotate_set_arrow(gboolean arrow)
{
  data->arrow = arrow;
}


/* Set the line thickness. */
void annotate_set_thickness(gdouble thickness)
{
  data->thickness = thickness;
}


/* Get the line thickness. */
gdouble annotate_get_thickness()
{
  if (data->cur_context->type != ANNOTATE_ERASER)
    {
      return data->thickness; 
    }
  return data->thickness*2.5; 
}


/* Add to the list of the painted point the point (x,y). */
void annotate_coord_list_prepend (gdouble x, gdouble y, gint width, gdouble pressure)
{
  AnnotateStrokeCoordinate *point;
  point = g_malloc ((gsize) sizeof (AnnotateStrokeCoordinate));
  point->x = x;
  point->y = y;
  point->width = width;
  point->pressure = pressure;
  data->coord_list = g_slist_prepend (data->coord_list, point);
}


/* Free the list of the painted point. */
void annotate_coord_list_free()
{
  if (data->coord_list)
    {
      g_slist_foreach(data->coord_list, (GFunc)g_free, NULL);
      g_slist_free(data->coord_list);
      data->coord_list = NULL;
    }
}


/* Modify color according to the pressure. */
void annotate_modify_color(AnnotateData* data, gdouble pressure)
{
  /* Pressure value is from 0 to 1; this value modify the RGBA gradient. */
  gint r,g,b,a;
  gdouble old_pressure = pressure;
  /* If you put an highter value you will have more contrast
   * beetween the lighter and darker color depending on pressure. 
   */
  gdouble contrast = 96;
  gdouble corrective = 0;
  
  /* The pressure is greater than 0. */
  if ((!data->annotation_cairo_context)||(!data->cur_context->fg_color))
    {
      return;
    }
  if (pressure >= 1)
    {
      cairo_set_source_color_from_string(data->annotation_cairo_context, data->cur_context->fg_color);
    }
  if (pressure <= 0.1)
    {
      pressure = 0.1;
    }
  sscanf (data->cur_context->fg_color, "%02X%02X%02X%02X", &r, &g, &b, &a);
  if (data->coord_list)
    { 
      AnnotateStrokeCoordinate* last_point = (AnnotateStrokeCoordinate*) g_slist_nth_data (data->coord_list, 0);
      old_pressure = last_point->pressure;      
    }
  corrective = (1-( 3 * pressure + old_pressure)/4) * contrast;
  cairo_set_source_rgba (data->annotation_cairo_context, (r + corrective)/255, 
			 (g + corrective)/255, 
			 (b+corrective)/255, 
			 (gdouble) a/255);
}


/* Set a new cairo path with the new options. */
void annotate_reset_cairo()
{
  if (data->annotation_cairo_context)
    {
      cairo_new_path(data->annotation_cairo_context);
    }
  configure_pen_options();  
}


/* Paint the context over the annotation window. */
void annotate_push_context(cairo_t * cr)
{
  if (data->debug)
    {
      g_printerr("The text window content has been painted over the annotation window\n");
    }

  annotate_reset_cairo(); 
  cairo_surface_t* source_surface = cairo_get_target(cr);

#ifndef _WIN32
  /*
   * The over operator might put the new layer on the top of the old one 
   * overriding the intersections
   * Seems that this operator does not work on windows
   * in this operating system only the new layer remain after the merge.
   *
   */
  cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_OVER);
#else
  /*
   * @WORKAROUND using CAIRO_OPERATOR_ADD instead of CAIRO_OPERATOR_OVER 
   * I do this to use the text widget in windows 
   * I use the CAIRO_OPERATOR_ADD that put the new layer
   * on the top of the old; this function does not preserve the color of
   * the second layer but modify it respecting the firts layer.
   *
   * Seems that the CAIRO_OPERATOR_OVER does not work because in the
   * gtk cairo implementation the ARGB32 format is not supported.
   *
   */
  cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_ADD);
#endif

  cairo_set_source_surface (data->annotation_cairo_context,  source_surface, 0, 0);
  cairo_paint(data->annotation_cairo_context);
  cairo_stroke(data->annotation_cairo_context);
  annotate_add_savepoint();
}


/* Select the default pen tool. */
void annotate_select_pen()
{
  if (data->debug)
    {
      g_printerr("The pen has been selected\n");
    }
  data->cur_context = data->default_pen;
  data->old_paint_type = ANNOTATE_PEN;
  annotate_set_pen_cursor();
}


/* Select the default eraser tool. */
void annotate_select_eraser()
{
  if (data->debug)
    {
      g_printerr("The eraser has been selected\n");
    } 
  data->cur_context = data->default_eraser;
  data->old_paint_type = ANNOTATE_ERASER;
  annotate_set_eraser_cursor();
}


/* Unhide the cursor. */
void annotate_unhide_cursor()
{
  if (data->is_cursor_hidden)
    {
      update_cursor();
      data->is_cursor_hidden = FALSE;
    }
}

 
/* Hide the cursor icon. */
void annotate_hide_cursor()
{
  gdk_window_set_cursor(data->annotation_window->window, data->invisible_cursor);
  data->is_cursor_hidden = TRUE;
}


/* acquire the grab. */
void annotate_acquire_grab()
{
  if  (!data->is_grabbed)
    {
      if (data->debug)
        {
	  g_printerr("Acquire grab\n");
	}
      annotate_acquire_input_grab();
      data->is_grabbed = TRUE;
    }
}


/* Draw line from the last point drawn to (x2,y2). */
void annotate_draw_line(gdouble x2, gdouble y2, gboolean stroke)
{
  if (!stroke)
    {
      cairo_line_to(data->annotation_cairo_context, x2, y2);
    }
  else
    {
      cairo_line_to(data->annotation_cairo_context, x2, y2);
      cairo_stroke(data->annotation_cairo_context);
      cairo_move_to(data->annotation_cairo_context, x2, y2);
    }
}


/* Draw an arrow using some polygons. */
void annotate_draw_arrow(gint distance)
{
  gint arrow_minimum_size = data->thickness * 2;
  gint lenght = g_slist_length(data->coord_list);
  gdouble direction = 0;
  gint i = 0;
  
  if (distance < arrow_minimum_size)
    {  
      return;
    }

  if (data->debug)
    {
      g_printerr("Draw arrow: ");
    }

  if (lenght < 2)
    {
      /* If it has lenght lesser then two then is a point and it has no sense draw the arrow. */
      return;
    }
  
  /* Postcondition lenght >= 2 */
  direction = annotate_get_arrow_direction();
  if (data->debug)
    {
      g_printerr("Arrow direction %f\n", direction/M_PI*180);
    }
	
  AnnotateStrokeCoordinate* point = (AnnotateStrokeCoordinate*) g_slist_nth_data(data->coord_list, i);

  gint pen_width = data->thickness;
  
  gdouble width_cos = pen_width * cos(direction);
  gdouble width_sin = pen_width * sin(direction);

  /* Vertex of the arrow. */
  gdouble arrow_head_0_x = point->x + width_cos;
  gdouble arrow_head_0_y = point->y + width_sin;

  /* Left point. */
  gdouble arrow_head_1_x = point->x - width_cos + width_sin ;
  gdouble arrow_head_1_y = point->y -  width_cos - width_sin ;

  /* Origin. */
  gdouble arrow_head_2_x = point->x - 0.8 * width_cos ;
  gdouble arrow_head_2_y = point->y - 0.8 * width_sin ;

  /* Right point. */
  gdouble arrow_head_3_x = point->x - width_cos - width_sin ;
  gdouble arrow_head_3_y = point->y +  width_cos - width_sin ;

  cairo_stroke(data->annotation_cairo_context); 
  cairo_save(data->annotation_cairo_context);

  /* Init cairo properties. */
  cairo_set_line_join(data->annotation_cairo_context, CAIRO_LINE_JOIN_MITER); 
  cairo_set_operator(data->annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(data->annotation_cairo_context, pen_width);

  /* Draw the arrow. */
  cairo_move_to(data->annotation_cairo_context, arrow_head_2_x, arrow_head_2_y); 
  cairo_line_to(data->annotation_cairo_context, arrow_head_1_x, arrow_head_1_y);
  cairo_line_to(data->annotation_cairo_context, arrow_head_0_x, arrow_head_0_y);
  cairo_line_to(data->annotation_cairo_context, arrow_head_3_x, arrow_head_3_y);

  cairo_close_path(data->annotation_cairo_context);
  cairo_fill_preserve(data->annotation_cairo_context);
  cairo_stroke(data->annotation_cairo_context);
  cairo_restore(data->annotation_cairo_context);
 
  if (data->debug)
    {
      g_printerr("with vertex at (x,y)=(%f : %f)\n",  arrow_head_0_x , arrow_head_0_y  );
    }
}


/* Draw a point in x,y respecting the context. */
void annotate_draw_point(gdouble x, gdouble y, gdouble pressure)
{
  /* Modify a little bit the color depending on pressure. */
  annotate_modify_color(data, pressure); 
  cairo_move_to(data->annotation_cairo_context, x, y);
  cairo_line_to(data->annotation_cairo_context, x, y);
}


/* Call the geometric shape recognizer. */
void annotate_shape_recognize(gboolean closed_path)
{
  /* Rectify only if the list is greater that 3. */
  if ( g_slist_length(data->coord_list)>3)
    {
      if (data->rectify)
        {
	  rectify(closed_path);
	} 
      else if (data->roundify)
	{
	  roundify(closed_path); 
	}
    }
}


/* Select eraser, pen or other tool for tablet. */
void annotate_select_tool(AnnotateData* data, GdkDevice *device, guint state)
{
  if (device)
    {
      if (device->source == GDK_SOURCE_ERASER)
	{
	  annotate_select_eraser();
	}
      else
	{
	  annotate_select_pen();
	}
    }
  else
    {
      g_printerr("Attempt to select nonexistent device!\n");
      data->cur_context = data->default_pen;
    } 
}


/* Free the memory allocated by paint context */
void annotate_paint_context_free(AnnotatePaintContext *context)
{
  if (context)
    {
      g_free(context);
      context=NULL;
    }
}


/* Quit the annotation. */
void annotate_quit()
{
  export_iwb(get_iwb_filename());
  if (data)
    {   

      /* Unref gtkbuilder. */
      if (data->annotation_window_gtk_builder)
	{
	  g_object_unref(data->annotation_window_gtk_builder);
	}

      if (data->shape)
	{  
	  g_object_unref(data->shape);
	}
	
      /* Destroy cairo. */  
      destroy_cairo();

      /* Destroy cursors. */  
      disallocate_cursor();
  
      if (data->invisible_cursor)
	{
	  gdk_cursor_unref(data->invisible_cursor);
	}

      /* Free all. */
      if (data->annotation_window)
	{
	  gtk_widget_destroy(data->annotation_window); 
	}

      annotate_coord_list_free();
      annotate_savepoint_list_free();

      gchar* ardesia_tmp_dir = g_build_filename(g_get_tmp_dir(), PACKAGE_NAME, (gchar *) 0);
      rmdir_recursive(ardesia_tmp_dir);
      g_free(ardesia_tmp_dir);

      g_free(data->savepoint_dir);
 
      annotate_paint_context_free(data->default_pen);
      annotate_paint_context_free(data->default_eraser);

      g_free(data);
    }
}


/* Release input grab; the input event will be passed below the window. */
void annotate_release_input_grab()
{
  gdk_window_set_cursor(data->annotation_window->window, NULL);
#ifndef _WIN32
  /* 
   * @TODO implement correctly gdk_window_input_shape_combine_mask 
   * in the quartz gdkwindow or use an equivalent native function;
   * the current implementation in macosx this doesn't do nothing.
   */
  /*
   * This allows the mouse event to be passed below the transparent annotation; 
   * at the moment this call works only on Linux  
   */
  gdk_window_input_shape_combine_mask (data->annotation_window->window, data->shape, 0, 0);
#else
  /*
   * @TODO WIN32 implement correctly gdk_window_input_shape_combine_mask 
   * in the win32 gdkwindow or use an equivalent native function.
   * Now in the gtk implementation the gdk_window_input_shape_combine_mask
   * call the gdk_window_shape_combine_mask that is not the desired behaviour.
   *
   */
  annotate_release_pointer_grab();
#endif  
}


/* Release the pointer pointer. */
void annotate_release_grab ()
{      
  if (data->is_grabbed)
    {
      if (data->debug)
	{
	  g_printerr("Release grab\n");
	}   
      annotate_release_input_grab();
      gtk_window_present(GTK_WINDOW(get_bar_window()));
      data->is_grabbed=FALSE;
    }
}


/* Fill the last shape. */
void annotate_fill()
{
  if (data->debug)
    {
      g_printerr("Fill\n");
    }
  if (data->coord_list)
    {
      if (is_a_closed_path(data->coord_list))
        {
          cairo_stroke(data->annotation_cairo_context);
          return;
        }
      if (!(data->roundify)&&(!(data->rectify)))
	{
	  annotate_draw_point_list(data->coord_list);     
	  cairo_close_path(data->annotation_cairo_context);      
	}
      select_color();
      cairo_fill(data->annotation_cairo_context);
      cairo_stroke(data->annotation_cairo_context);
      if (data->debug)
        {
	  g_printerr("Fill\n");
        }
      annotate_add_savepoint();
    }
}


/* Undo reverting to the last save point. */
void annotate_undo()
{
  if (data->savepoint_list)
    {
      if (data->current_save_index != g_slist_length(data->savepoint_list)-1)
        {
	  if (data->debug)
	    {
	      g_printerr("Undo\n");
	    }
	  data->current_save_index = data->current_save_index + 1;
	  annotate_restore_surface();
        }
    }
}


/* Redo to the last save point. */
void annotate_redo()
{
  if (data->savepoint_list)
    {
      if (data->current_save_index != 0)
        {
	  if (data->debug)
	    {
	      g_printerr("Redo\n");
	    }
	  data->current_save_index = data->current_save_index - 1;
	  annotate_restore_surface();
        }
    }
}


/* Clear the annotations windows. */
void annotate_clear_screen()
{
  if (data->debug)
    {
      g_printerr("Clear screen\n");
    }

  annotate_reset_cairo();
  clear_cairo_context(data->annotation_cairo_context);
  annotate_add_savepoint();
}


/* Init the annotation. */
gint annotate_init(GtkWidget* parent, gchar* iwb_file, gboolean debug)

{
  data = g_malloc ((gsize) sizeof(AnnotateData));
 
  /* Init the data structure. */ 
  data->annotation_cairo_context = NULL;
  data->coord_list = NULL;
  data->savepoint_list = NULL;
  data->current_save_index = 0;
  data->cursor = NULL;

  data->is_grabbed = FALSE;
  data->arrow = FALSE; 
  data->rectify = FALSE;
  data->roundify = FALSE;

  data->is_cursor_hidden = TRUE;

  data->debug = debug;
  
  allocate_invisible_cursor();
  
  create_savepoint_dir();

  if (iwb_file)
    {
      data->savepoint_list = load_iwb(iwb_file);
    }
  
  setup_app(parent);

  return 0;
}


