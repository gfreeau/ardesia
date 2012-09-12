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

/* Widget for text insertion */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <utils.h>
#include <annotation_window.h>
#include <text_window.h>
#include <keyboard.h>

#ifdef _WIN32
#  include <windows_utils.h>
#endif


/* The structure used internally to store the status. */
static TextData *text_data = (TextData *) NULL;

/* The structure used to configure text input. */
TextConfig *text_config = (TextConfig *) NULL;

/* Create the text window. */
static void
create_text_window           (GtkWindow *parent)
{
  GError *error = (GError *) NULL;

  if (!text_data->text_window_gtk_builder)
    {
      /* Initialize the main window. */
      text_data->text_window_gtk_builder = gtk_builder_new ();

      /* Load the gtk builder file created with glade. */
      gtk_builder_add_from_file (text_data->text_window_gtk_builder, TEXT_UI_FILE, &error);

      if (error)
        {
          g_warning ("Failed to load builder file: %s", error->message);
          g_error_free (error);
          return;
        }

    }

  if (!text_data->window)
    {
      GObject *text_obj = gtk_builder_get_object (text_data->text_window_gtk_builder, "text_window");
      text_data->window = GTK_WIDGET (text_obj);

      gtk_window_set_opacity (GTK_WINDOW (text_data->window), 1);
      gtk_widget_set_size_request (GTK_WIDGET (text_data->window), gdk_screen_width (), gdk_screen_height ());
    }

}


/* Move the pen cursor. */
static void move_editor_cursor    ()
{
  if (text_data->cr)
    {
      cairo_move_to (text_data->cr, text_data->pos->x, text_data->pos->y);
    }
}


/* Blink cursor. */
static
gboolean blink_cursor        (gpointer data)
{
  if ((text_data->window) && (text_data->pos))
    {
      cairo_t *cr = gdk_cairo_create (gtk_widget_get_window  (text_data->window));
      gint height = text_data->max_font_height;

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

      if (text_data->blink_show)
        {
          cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
          cairo_set_line_width (cr, text_data->pen_width);
          cairo_set_source_color_from_string (cr, text_data->color);
          cairo_rectangle (cr, text_data->pos->x, text_data->pos->y - height, TEXT_CURSOR_WIDTH, height);
          text_data->blink_show = FALSE;
        }
      else
        {
          cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
          cairo_rectangle (cr, text_data->pos->x, text_data->pos->y - height, TEXT_CURSOR_WIDTH, height);

          cairo_rectangle (cr,
                           text_data->pos->x-1,
                           text_data->pos->y - height - 1,
                           TEXT_CURSOR_WIDTH  + 2,
                           height + 2);
          text_data->blink_show=TRUE;
        }

      cairo_fill (cr);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  return TRUE;
}


/* Delete the last character printed. */
static void
delete_character        ()
{
  CharInfo *char_info = (CharInfo *) g_slist_nth_data (text_data->letterlist, 0);

  if (char_info)
    {
      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_CLEAR);

      cairo_rectangle (text_data->cr,
                       char_info->x + char_info->x_bearing -1,
                       char_info->y + char_info->y_bearing -1,
                       gdk_screen_width ()+2,
                       text_data->max_font_height + 2);

      cairo_fill (text_data->cr);
      cairo_stroke (text_data->cr);
      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_SOURCE);
      text_data->pos->x = char_info->x;
      text_data->pos->y = char_info->y;
      text_data->letterlist = g_slist_remove (text_data->letterlist, char_info);
    }

}


/* Stop the timer to handle the blocking cursor. */
static void
stop_timer              ()
{
  if (text_data->timer>0)
    {
      g_source_remove (text_data->timer);
      text_data->timer = -1;
    }
}


/* Set the text cursor. */
static gboolean
set_text_cursor              (GtkWidget  *window)
{
  gdouble decoration_height = 4;
  gint height = text_data->max_font_height + decoration_height * 2;
  gint width = TEXT_CURSOR_WIDTH * 3;  
  cairo_surface_t *text_surface_t = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *text_pointer_cr = cairo_create (text_surface_t);
  GdkColor *background_color_p = rgba_to_gdkcolor (BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor (text_data->color);
  GdkCursor *cursor = (GdkCursor *) NULL;
  GdkPixbuf *pixbuf = (GdkPixbuf *) NULL;

  if (text_pointer_cr)
    {
      clear_cairo_context (text_pointer_cr);
      cairo_set_source_color_from_string (text_pointer_cr, text_data->color);
      cairo_set_operator (text_pointer_cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_line_width (text_pointer_cr, 2);

      cairo_line_to (text_pointer_cr, 1, 1);
      cairo_line_to (text_pointer_cr, width-1, 1);
      cairo_line_to (text_pointer_cr, width-1, decoration_height);
      cairo_line_to (text_pointer_cr, 2*width/3+1, decoration_height);
      cairo_line_to (text_pointer_cr, 2*width/3+1, height-decoration_height);
      cairo_line_to (text_pointer_cr, width-1, height-decoration_height);
      cairo_line_to (text_pointer_cr, width-1, height-1);
      cairo_line_to (text_pointer_cr, 1, height-1);
      cairo_line_to (text_pointer_cr, 1, height-decoration_height);
      cairo_line_to (text_pointer_cr, width/3-1, height-decoration_height);
      cairo_line_to (text_pointer_cr, width/3-1, decoration_height);
      cairo_line_to (text_pointer_cr, 1, decoration_height);
      cairo_close_path (text_pointer_cr);

      cairo_stroke (text_pointer_cr);
      cairo_destroy (text_pointer_cr);
    }

  pixbuf = gdk_pixbuf_get_from_surface (text_surface_t,
                                        0,
                                        0,
                                        width,
                                        height);

  cursor = gdk_cursor_new_from_pixbuf ( gdk_display_get_default (), pixbuf, 0, 0);

  gdk_window_set_cursor (gtk_widget_get_window  (text_data->window), cursor);
  gdk_flush ();
  g_object_unref (cursor);
  g_object_unref (pixbuf);
  cairo_surface_destroy (text_surface_t);
  g_free (foreground_color_p);
  g_free (background_color_p);
  
  return TRUE;
}


/* Add a save-point with the text. */
static void
save_text          ()
{
  if (text_data)
    {
      stop_timer (); 
      text_data->blink_show=FALSE;
      blink_cursor (NULL);

      if (text_data->letterlist)
        {
          annotate_push_context (text_data->cr);
          g_slist_foreach (text_data->letterlist,
                           (GFunc)g_free,
                           NULL);

          g_slist_free (text_data->letterlist);
          text_data->letterlist = NULL;
        }

    }
}


/* Initialization routine. */
static void
init_text_widget             (GtkWidget *widget)
{
  gtk_widget_input_shape_combine_region(text_data->window, NULL);
  drill_window_in_bar_area (text_data->window);

  if (!text_data->cr)
    {
      text_data->cr = gdk_cairo_create ( gtk_widget_get_window (widget) );
      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_line_width (text_data->cr, text_data->pen_width);
      cairo_set_source_color_from_string (text_data->cr, text_data->color);
      cairo_set_font_size (text_data->cr, text_data->pen_width * 2);
      
      /* Select the font */
      cairo_select_font_face (text_data->cr, text_config->fontfamily,
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);

      /* This is a trick; we must found the maximum height of the font. */
      cairo_text_extents (text_data->cr, "|" , &text_data->extents);
      text_data->max_font_height = text_data->extents.height;
      set_text_cursor (widget);
    }

#ifdef _WIN32
  grab_pointer (text_data->window, TEXT_MOUSE_EVENTS);
#endif
  
  if (!text_data->letterlist)
    {
      clear_cairo_context (text_data->cr);
    }
  
  if (!text_data->pos)
    {
      text_data->pos = g_malloc ( (gsize) sizeof (Pos));
      text_data->pos->x = 0;
      text_data->pos->y = 0;
      move_editor_cursor ();
    }
}


/* Destroy text window. */
static void
destroy_text_window     ()
{
  if (text_data->window)
    {
#ifdef _WIN32
      ungrab_pointer (gdk_display_get_default ());
#endif
      if (text_data->window)
        {
          gtk_widget_destroy (text_data->window);
          text_data->window = NULL;
        }
    }
}


/* keyboard event snooper. */
G_MODULE_EXPORT gboolean
key_snooper                  (GtkWidget    *widget,
                              GdkEventKey  *event,
                              gpointer      user_data)
{

  if (event->type != GDK_KEY_PRESS) {
    return TRUE;
  }

  stop_timer ();
  text_data->blink_show=FALSE;
  blink_cursor (NULL);

  gboolean closed_to_bar = inside_bar_window (text_data->pos->x + text_data->extents.x_advance,
                                              text_data->pos->y-text_data->max_font_height/2);

  if ( (event->keyval == GDK_KEY_BackSpace) ||
       (event->keyval == GDK_KEY_Delete))
    {
      delete_character (); // undo the last character inserted
    }
  /* It is the end of the line or the letter is closed to the window bar. */
  else if ( (text_data->pos->x + text_data->extents.x_advance >= gdk_screen_width ()) ||
	    (closed_to_bar) ||
	    (event->keyval == GDK_KEY_Return) ||
	    (event->keyval == GDK_KEY_ISO_Enter) ||
	    (event->keyval == GDK_KEY_KP_Enter))
    {
     /* select the x indentation */
      text_data->pos->x = text_config->leftmargin;
      text_data->pos->y +=  text_data->max_font_height;
    }
  /* Simple Tab-Implementation */
  else if (event->keyval == GDK_KEY_Tab)
    {
      text_data->pos->x += text_config->tabsize;
    }
  /* Is the character printable? */
  else if (isprint (event->keyval))
    {
      /* Postcondition: the character is printable. */
      gchar *utf8 = g_strdup_printf ("%c", event->keyval);
      
      CharInfo *char_info = g_malloc ( (gsize) sizeof (CharInfo));
      char_info->x = text_data->pos->x;
      char_info->y = text_data->pos->y;
      
      cairo_text_extents (text_data->cr, utf8, &text_data->extents);
      cairo_show_text (text_data->cr, utf8);
      cairo_stroke (text_data->cr);

      char_info->x_bearing = text_data->extents.x_bearing;
      char_info->y_bearing = text_data->extents.y_bearing;

      text_data->letterlist = g_slist_prepend (text_data->letterlist, char_info);
      
      /* Move cursor to the x step */
      text_data->pos->x +=  text_data->extents.x_advance;
      
      g_free (utf8);
    }

  move_editor_cursor ();
  text_data->blink_show=TRUE;
  blink_cursor (NULL);
  text_data->timer = g_timeout_add (1000, blink_cursor, NULL);

  return TRUE;
}


/* On configure event. */
G_MODULE_EXPORT gboolean
on_text_window_configure     (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data)
{
  return TRUE;
}


/* On screen changed. */
G_MODULE_EXPORT void
on_text_window_screen_changed     (GtkWidget  *widget,
                                   GdkScreen  *previous_screen,
                                   gpointer    user_data)
{
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET (widget));
  GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
  if (visual == NULL)
    {
      visual = gdk_screen_get_system_visual (screen);
    }

  gtk_widget_set_visual (widget, visual);
}


/* The windows has been exposed. */
G_MODULE_EXPORT gboolean
on_text_window_expose_event  (GtkWidget  *widget,
                              cairo_t    *cr,
                              gpointer    data)
{
  GdkWindow *gdk_win = gtk_widget_get_window (widget);
  gint is_fullscreen = gdk_window_get_state (gdk_win) & GDK_WINDOW_STATE_FULLSCREEN;

  if (!is_fullscreen)
    {
      return FALSE;
    }

  if (widget)
    {
      init_text_widget (widget);
    }

  return TRUE;
}


#ifdef _WIN32
/* Is the point (x,y) above the virtual keyboard? */
static gboolean
is_above_virtual_keyboard    (gint  x,
                              gint  y)
{
  RECT rect;
  HWND hwnd = FindWindow (VIRTUALKEYBOARD_WINDOW_NAME, NULL);
  if (!hwnd)
    {
      return FALSE;
    }
  if (!GetWindowRect (hwnd, &rect))
    {
      return FALSE;
    }
  if ( (rect.left<x)&& (x<rect.right)&& (rect.top<y)&& (y<rect.bottom))
    {
      return TRUE;
    }
  return FALSE;
}
#endif


/* This is called when the button is leased. */
G_MODULE_EXPORT gboolean
on_text_window_button_release     (GtkWidget       *win,
                                   GdkEventButton  *ev,
                                   gpointer         user_data)
{
  /* only button1 allowed */
  if (ev->button != 1)
    {
      return TRUE;
    }

#ifdef _WIN32
  gboolean above = is_above_virtual_keyboard (ev->x_root, ev->y_root);

  if (above)
    {
      /* You have lost the focus; re grab it. */
      grab_pointer (text_data->window, TEXT_MOUSE_EVENTS);
      /* Ignore the data; the event will be passed to the virtual keyboard. */
      return TRUE;
    }

#endif

  if ((text_data) && (text_data->pos))
    {
      save_text ();

      text_data->pos->x = ev->x_root;
      text_data->pos->y = ev->y_root;
      move_editor_cursor ();

      /* This present the ardesia bar and the panels. */
      gtk_window_present (GTK_WINDOW (get_bar_window ()));
      gtk_window_present (GTK_WINDOW (text_data->window));
      gdk_window_raise (gtk_widget_get_window  (text_data->window));

      stop_virtual_keyboard ();
      start_virtual_keyboard ();

      text_data->timer = g_timeout_add (1000, blink_cursor, NULL);
    }

  return TRUE;
}


/* This shots when the text pointer is moving. */
G_MODULE_EXPORT gboolean
on_text_window_cursor_motion      (GtkWidget       *win,
                                   GdkEventMotion  *ev,
                                   gpointer         func_data)
{
#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {
      stop_text_widget ();
    }
#endif
  return TRUE;
}


/* Start the widget for the text insertion. */
void start_text_widget      (GtkWindow  *parent,
                             gchar      *color,
                             gint        tickness)
{
  text_data = g_malloc ((gsize) sizeof (TextData));

  text_data->text_window_gtk_builder = NULL;
  text_data->window = NULL;
  text_data->cr = NULL;
  text_data->pos = NULL;
  text_data->letterlist = NULL;

  text_data->virtual_keyboard_pid = (GPid) 0;
  text_data->snooper_handler_id = 0;
  text_data->timer = -1;
  text_data->blink_show = TRUE;

  text_data->color =  color;
  text_data->pen_width = tickness;

  create_text_window (parent);

  /* This trys to set an alpha channel. */
  on_text_window_screen_changed(text_data->window, NULL, text_data);
  
  /* In the gtk 2.16.6 the gtkbuilder property double-buffered is not parsed
   * from the glade file and then I set this by hands. 
   */
  gtk_widget_set_double_buffered (text_data->window, FALSE); 

  //gtk_window_set_transient_for (GTK_WINDOW (text_data->window), GTK_WINDOW (parent));
  gtk_window_set_keep_above (GTK_WINDOW (text_data->window), TRUE);
  gtk_widget_grab_focus (text_data->window);

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals (text_data->text_window_gtk_builder, (gpointer) text_data);

  /* Install the key snooper. */
  text_data->snooper_handler_id = gtk_key_snooper_install (key_snooper, NULL);

  /* This put the window in fullscreen generating an exposure. */
  gtk_window_fullscreen (GTK_WINDOW (text_data->window));

  gtk_widget_show_all (text_data->window);  
#ifdef _WIN32 
  /* I use a layered window that use the black as transparent color. */
  setLayeredGdkWindowAttributes (gtk_widget_get_window  (text_data->window), RGB (0,0,0), 0, LWA_COLORKEY);	
#endif
}


/* Stop the text insertion widget. */
void
stop_text_widget             ()
{
  if (text_data)
    {
      stop_virtual_keyboard ();

      if (text_data->snooper_handler_id)
        {
          gtk_key_snooper_remove (text_data->snooper_handler_id);
          text_data->snooper_handler_id = 0;
        }

      save_text ();
      destroy_text_window ();

      if (text_data->cr)
        {
          cairo_destroy (text_data->cr);
          text_data->cr = NULL;
        }

      if (text_data->pos)
        {
          g_free (text_data->pos);
          text_data->pos = NULL;
        }

      /* Free the gtk builder object. */
      if (text_data->text_window_gtk_builder)
        {
          g_object_unref (text_data->text_window_gtk_builder);
          text_data->text_window_gtk_builder = NULL;
        }

      g_free (text_data);
      text_data = NULL;
    }
}


