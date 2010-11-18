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

/* Widget for text insertion */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <text_widget.h>
#include <utils.h>
#include <annotate.h>

#ifdef _WIN32
#include <windows_utils.h>
#endif


static TextData* text_data;


void start_virtual_keyboard()
{ 
  gchar* argv[2] = {VIRTUALKEYBOARD_NAME, (gchar*) 0};


  g_spawn_async (NULL /*working_directory*/,
		 argv,
		 NULL /*envp*/,
		 G_SPAWN_SEARCH_PATH,
		 NULL /*child_setup*/,
		 NULL /*user_data*/,
		 &text_data->virtual_keyboard_pid /*child_pid*/,
		 NULL /*error*/);
}


void stop_virtual_keyboard()
{
  if (text_data->virtual_keyboard_pid>0)
    {
      g_spawn_close_pid(text_data->virtual_keyboard_pid);  
      /* @TODO replace this with the cross plattform g_pid_terminate when it will available */
#ifdef _WIN32
      TerminateProcess((HANDLE) text_data->virtual_keyboard_pid, 0)
#else
	kill (text_data->virtual_keyboard_pid, SIGTERM);
#endif   
      text_data->virtual_keyboard_pid = (GPid) 0;
    }
}


/* Creation of the text window */
void create_text_window(GtkWindow *parent)
{
  text_data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  gtk_widget_set_usize (GTK_WIDGET(text_data->window), gdk_screen_width(), gdk_screen_height());
  gtk_window_fullscreen(GTK_WINDOW(text_data->window));

  gtk_window_set_decorated(GTK_WINDOW(text_data->window), FALSE);
  gtk_widget_set_app_paintable(text_data->window, TRUE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(text_data->window), TRUE);
  gtk_window_set_opacity(GTK_WINDOW(text_data->window), 1); 
  
  gtk_widget_set_double_buffered(text_data->window, FALSE);
}


/* Move the pen cursor */
void move_editor_cursor()
{
  if (text_data->cr)
    {
      cairo_move_to(text_data->cr, text_data->pos->x, text_data->pos->y);
    }
}


/** Delete the last character printed */
void delete_character()
{
  CharInfo *charInfo = (CharInfo *) g_slist_nth_data (text_data->letterlist, 0);

  if (charInfo)
    {  

      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_CLEAR);
 
      cairo_rectangle(text_data->cr, charInfo->x + charInfo->x_bearing, 
                      charInfo->y + charInfo->y_bearing, 
                      gdk_screen_width(), 
                      text_data->max_font_height + text_data->pen_width + 3);

      cairo_fill(text_data->cr);
      cairo_stroke(text_data->cr);
      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_SOURCE);
      text_data->pos->x = charInfo->x;
      text_data->pos->y = charInfo->y;
      move_editor_cursor();
      text_data->letterlist = g_slist_remove(text_data->letterlist,charInfo); 
    }
}


/* keyboard event snooper */
G_MODULE_EXPORT gboolean
key_snooper(GtkWidget *widget, GdkEventKey *event, gpointer user_data)  
{
  if (event->type != GDK_KEY_PRESS) {
    return TRUE;
  }
  
  if ((event->keyval == GDK_BackSpace) ||
      (event->keyval == GDK_Delete))
    {
      // undo
      delete_character();
      return TRUE;
    }
  /* is finished the line or the letter is near to the bar window */
  if ((text_data->pos->x + text_data->extents.x_advance >= gdk_screen_width()) ||
      (inside_bar_window(text_data->pos->x + text_data->extents.x_advance, text_data->pos->y-text_data->max_font_height/2)) ||
      (event->keyval == GDK_Return) ||
      (event->keyval == GDK_ISO_Enter) || 	
      (event->keyval == GDK_KP_Enter)
      )
    {
      text_data->pos->x = 0;
      text_data->pos->y +=  text_data->max_font_height;

      /* go to the new line */
      move_editor_cursor();  
    }
  /* is the character printable? */
  else if (isprint(event->keyval))
    {
      /* The character is printable */
      gchar *utf8 = g_strdup_printf("%c", event->keyval);
      
      CharInfo *charInfo = g_malloc(sizeof (CharInfo));
      charInfo->x = text_data->pos->x;
      charInfo->y = text_data->pos->y; 
      
      cairo_text_extents (text_data->cr, utf8, &text_data->extents);
      cairo_show_text (text_data->cr, utf8); 
      cairo_stroke(text_data->cr);
 
      charInfo->x_bearing = text_data->extents.x_bearing;
      charInfo->y_bearing = text_data->extents.y_bearing;
     
      text_data->letterlist = g_slist_prepend (text_data->letterlist, charInfo);
      
      /* move cursor to the x step */
      text_data->pos->x +=  text_data->extents.x_advance;
      move_editor_cursor();  
      
      g_free(utf8);
    }
  return TRUE;
}


/* Set the text cursor */
gboolean set_text_cursor(GtkWidget * window)
{
  
  gint height = text_data->max_font_height;
  gint width = text_data->pen_width;
  
  GdkPixmap *pixmap = gdk_pixmap_new (NULL, width , height, 1);
  cairo_t *text_pointer_pcr = gdk_cairo_create(pixmap);
  cairo_set_operator(text_pointer_pcr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgb(text_pointer_pcr, 1, 1, 1);
  cairo_paint(text_pointer_pcr);
  cairo_stroke(text_pointer_pcr);
  cairo_destroy(text_pointer_pcr);
  
  GdkPixmap *bitmap = gdk_pixmap_new (NULL, width , height, 1);  
  cairo_t *text_pointer_cr = gdk_cairo_create(bitmap);
  
  if (text_pointer_cr)
    {
      clear_cairo_context(text_pointer_cr);
      cairo_set_source_rgb(text_pointer_cr, 1, 1, 1);
      cairo_set_line_cap (text_pointer_cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(text_pointer_cr, CAIRO_LINE_JOIN_ROUND); 
      cairo_set_operator(text_pointer_cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_line_width(text_pointer_cr, text_data->pen_width);
      cairo_rectangle(text_pointer_cr, 0, 0, 5, height);  
      cairo_stroke(text_pointer_cr);
      cairo_destroy(text_pointer_cr);
    } 
 
  GdkColor *background_color_p = rgba_to_gdkcolor(BLACK);
  GdkColor *foreground_color_p = rgba_to_gdkcolor(text_data->color);
  
  GdkCursor* cursor = gdk_cursor_new_from_pixmap (pixmap, bitmap, foreground_color_p, background_color_p, 1, height-1);
  gdk_window_set_cursor (text_data->window->window, cursor);
  gdk_flush ();
  gdk_cursor_unref(cursor);
  g_object_unref (pixmap);
  g_object_unref (bitmap);
  g_free(foreground_color_p);
  g_free(background_color_p);
  
  return TRUE;
}


/* Initialization routine */
void init_text_widget(GtkWidget *widget)
{
  if (text_data->cr==NULL)
    {
      text_data->cr = gdk_cairo_create(widget->window);
      cairo_set_line_cap (text_data->cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(text_data->cr, CAIRO_LINE_JOIN_ROUND); 
      cairo_set_operator(text_data->cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_line_width(text_data->cr, text_data->pen_width);
      cairo_set_source_color_from_string(text_data->cr, text_data->color);
      /* This is a trick we must found the maximum height and text_pen_width of the font */
      cairo_set_font_size (text_data->cr, text_data->pen_width * 2);
      cairo_text_extents (text_data->cr, "|" , &text_data->extents);
      text_data->max_font_height = text_data->extents.height;
      set_text_cursor(widget);
#ifdef _WIN32
      grab_pointer(text_data->window, TEXT_MOUSE_EVENTS);
#endif
    }
  
  clear_cairo_context(text_data->cr);

  if (!text_data->pos)
    {
      text_data->pos = g_malloc (sizeof(Pos));
      text_data->pos->x = 0;
      text_data->pos->y = 0;
      move_editor_cursor();
    }
}


/* The windows has been exposed */
G_MODULE_EXPORT gboolean
on_window_text_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  gint is_fullscreen = gdk_window_get_state (widget->window) & GDK_WINDOW_STATE_FULLSCREEN;
  if (!is_fullscreen)
    {
      return TRUE;
    }
  if (widget)
    {
      init_text_widget(widget);
    }
  return TRUE;
}


/* Add a savepoint with the text */
void save_text()
{
  if (text_data->letterlist)
    {
      annotate_push_context(text_data->cr);
      g_slist_foreach(text_data->letterlist, (GFunc)g_free, NULL);
      g_slist_free(text_data->letterlist);
      text_data->letterlist = NULL;
    } 
}


/* This is called when the button is lease */
G_MODULE_EXPORT gboolean
on_window_text_button_release (GtkWidget *win,
			       GdkEventButton *ev, 
			       gpointer user_data)
{

  save_text();
#ifdef _WIN32
  ungrab_pointer(gdk_display_get_default(), text_data->window);
#endif
  
  text_data->pos->x = ev->x;
  text_data->pos->y = ev->y;
  move_editor_cursor();

  if (text_data->virtual_keyboard_pid!=0)
    {
      stop_virtual_keyboard();
    } 
  start_virtual_keyboard();
  
  /* This present the ardesia bar and the panels */
  gtk_window_present(GTK_WINDOW(get_bar_window()));

  gtk_window_present(GTK_WINDOW(text_data->window));

  return TRUE;
}


/* Start the widget for the text insertion */
void start_text_widget(GtkWindow *parent, gchar* color, gint tickness)
{
  text_data = g_malloc(sizeof(TextData));

  text_data->virtual_keyboard_pid = (GPid) 0;
  text_data->snooper_handler_id = 0;
  text_data->window = NULL;
  text_data->cr = NULL;

  text_data->pos = NULL;

  text_data->letterlist = NULL; 
  
  text_data->color =  color;
  text_data->pen_width = tickness;

  create_text_window(parent);
  gtk_window_set_keep_above(GTK_WINDOW(text_data->window), TRUE);

  gtk_widget_set_events (text_data->window, TEXT_MOUSE_EVENTS);

  g_signal_connect(G_OBJECT(text_data->window), "expose-event", G_CALLBACK(on_window_text_expose_event), NULL);
  g_signal_connect (G_OBJECT(text_data->window), "button_release_event", G_CALLBACK(on_window_text_button_release), NULL);
  /* install a key snooper */
  text_data->snooper_handler_id = gtk_key_snooper_install(key_snooper, NULL);
  

  gtk_widget_show_all(text_data->window);

#ifdef _WIN32 
  // I use a layered window that use the black as transparent color
  setLayeredGdkWindowAttributes(text_data->window->window, RGB(0,0,0), 0, LWA_COLORKEY);	
#endif
}


/* Stop the text insertion widget */
void stop_text_widget()
{
  if (text_data)
    {
      stop_virtual_keyboard();
      if (text_data->snooper_handler_id)
	{
	  gtk_key_snooper_remove(text_data->snooper_handler_id);
	}
      if (text_data->cr)
	{
	  save_text();
	  cairo_destroy(text_data->cr);     
	}
      if (text_data->window)
	{
	  gtk_widget_destroy(text_data->window);
	}
      if (text_data->pos)
	{
	  g_free(text_data->pos);
	}
      g_free(text_data);
      text_data = NULL;
    }
}


