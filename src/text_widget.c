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

#ifdef WIN32
  #include <windows_utils.h>
#endif

GtkWidget* text_window = NULL;
cairo_t *text_cr = NULL;

GdkDisplay* display = NULL;
GdkScreen* screen = NULL;
int screen_width;

Pos* pos = NULL;

GSList *letterlist = NULL; 
int text_pen_width = 16;

int max_font_height;


char* text_color = NULL;

GdkCursor* text_cursor;

cairo_text_extents_t extents;


/* Creation of the text window */
void create_text_window(GtkWindow *parent)
{
  text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for(GTK_WINDOW(text_window), parent);

  gtk_widget_set_usize (GTK_WIDGET(text_window), gdk_screen_width(), gdk_screen_height());
  gtk_window_fullscreen(GTK_WINDOW(text_window));

  gtk_window_set_decorated(GTK_WINDOW(text_window), FALSE);
  gtk_widget_set_app_paintable(text_window, TRUE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(text_window), TRUE);
  gtk_window_set_opacity(GTK_WINDOW(text_window), 1); 
  
  gtk_widget_set_double_buffered(text_window, FALSE);
}


/* Move the pen cursor */
void move_editor_cursor()
{
  if (text_cr)
    {
       cairo_move_to(text_cr, pos->x, pos->y);
    }
}


/** Delete the last character printed */
void delete_character()
{
  CharInfo *charInfo = (CharInfo *) g_slist_nth_data (letterlist, 0);

  if (charInfo)
    {  

      cairo_set_operator (text_cr, CAIRO_OPERATOR_CLEAR);
 
      cairo_rectangle(text_cr, charInfo->x + charInfo->x_bearing, 
                      charInfo->y + charInfo->y_bearing, 
                      screen_width, 
                      max_font_height + text_pen_width + 3);

      cairo_fill(text_cr);
      cairo_stroke(text_cr);
      cairo_set_operator (text_cr, CAIRO_OPERATOR_SOURCE);
      pos->x = charInfo->x;
      pos->y = charInfo->y;
      move_editor_cursor();
      letterlist = g_slist_remove(letterlist,charInfo); 
    }
}


/* Press keyboard event */
G_MODULE_EXPORT gboolean
key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)  
{
  if ((event->keyval == GDK_BackSpace)
      || (event->keyval == GDK_Delete))
    {
      // undo
      delete_character();
      return FALSE;
    }
  /* is finished the line */
  if ((pos->x + extents.x_advance >= screen_width))
    {
      pos->x = 0;
      pos->y +=  max_font_height;

      /* go to the new line */
      move_editor_cursor();  
    }
  /* is pressed enter */
  else if ((event->keyval == GDK_Return) ||
	   (event->keyval == GDK_ISO_Enter) || 	
	   (event->keyval == GDK_KP_Enter))
    {
      stop_text_widget();
    } 
  else if (isprint(event->keyval))
    {
      /* The character is printable */
      char *utf8 = g_malloc(2 * sizeof(char)) ;
      sprintf(utf8,"%c", event->keyval);
      
      CharInfo *charInfo = g_malloc(sizeof (CharInfo));
      charInfo->x = pos->x;
      charInfo->y = pos->y; 
      
      cairo_text_extents (text_cr, utf8, &extents);
      cairo_show_text (text_cr, utf8); 
      cairo_stroke(text_cr);
 
      charInfo->x_bearing = extents.x_bearing;
      charInfo->y_bearing = extents.y_bearing;
     
      letterlist = g_slist_prepend (letterlist, charInfo);
      
      /* move cursor to the x step */
      pos->x +=  extents.x_advance;
      move_editor_cursor();  
      
      g_free(utf8);
    }
  return TRUE;
}


/* Set the text cursor */
gboolean set_text_pointer(GtkWidget * window)
{
  
  int height = max_font_height;
  int width = text_pen_width;
  
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
       cairo_set_line_width(text_pointer_cr, text_pen_width);
       cairo_rectangle(text_pointer_cr, 0, 0, 5, height);  
       cairo_stroke(text_pointer_cr);
       cairo_destroy(text_pointer_cr);
    } 
 
  GdkColor *background_color_p = rgb_to_gdkcolor("000000");
  GdkColor *foreground_color_p = rgb_to_gdkcolor(text_color);
  if (text_cursor)
    {
      gdk_cursor_unref(text_cursor);
    }
  
  text_cursor = gdk_cursor_new_from_pixmap (pixmap, bitmap, foreground_color_p, background_color_p, 1, height-1);
  gdk_window_set_cursor (text_window->window, text_cursor);
  gdk_flush ();
  g_object_unref (pixmap);
  g_object_unref (bitmap);
  g_free(foreground_color_p);
  g_free(background_color_p);
  
  return TRUE;
}


/* Initialization routine */
void init(GtkWidget *widget)
{
  if (!text_cr)
    {
      text_cr = gdk_cairo_create(widget->window);
    }

  clear_cairo_context(text_cr);
  cairo_set_line_cap (text_cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(text_cr, CAIRO_LINE_JOIN_ROUND); 
  cairo_set_operator(text_cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(text_cr, text_pen_width);
  cairo_set_source_color_from_string(text_cr, text_color);

  if (!pos)
   {
      pos = g_malloc (sizeof(Pos));
   }
  pos->x = 0;
  pos->y = 0;
  move_editor_cursor();
  
}


/* The windows has been exposed */
G_MODULE_EXPORT gboolean
on_window_text_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  int is_fullscreen = gdk_window_get_state (widget->window) & GDK_WINDOW_STATE_FULLSCREEN;
  if (!is_fullscreen)
    {
      return TRUE;
    }
  if (!display)
    {
      display = gdk_display_get_default ();
      screen = gdk_display_get_default_screen (display);
      screen_width = gdk_screen_get_width (screen);
    }
  if (widget)
    {
      init(widget);
      if (text_cr)
        {
           /* This is a trick we must found the maximum height and text_pen_width of the font */
           cairo_set_font_size (text_cr, text_pen_width * 2);
           cairo_text_extents (text_cr, "|" , &extents);
           max_font_height = extents.height;
        }
	  set_text_pointer(widget);
	  #ifdef _WIN32
	    grab_pointer(text_window, TEXT_MOUSE_EVENTS);
          #endif
    }
  return TRUE;
}


/* This is called when the button is lease */
G_MODULE_EXPORT gboolean
release (GtkWidget *win,
         GdkEventButton *ev, 
         gpointer user_data)
{
 
  #ifdef _WIN32
    ungrab_pointer(display, text_window);
  #endif
  
  pos->x = ev->x;
  pos->y = ev->y;
  move_editor_cursor();
    
  gdk_window_set_cursor (text_window->window, NULL);
  
  #ifdef _WIN32  
    /*
        TODO try a way to pass the mouse input below the window
        the gdk_window_input_shape_combine_mask is not correctly implemented
     */
  #else
    /*
        Apply a transparent window with the gdk_window_input_shape_combine_mask
        to pass the mouse events below the text window  
     */
    int width, height;
    gtk_window_get_size(GTK_WINDOW(win), &width, &height);
      
    /* Initialize a transparent pixmap with depth 1 to be used as input shape */
    GdkPixmap* shape = gdk_pixmap_new (NULL, width, height, 1); 
    cairo_t* shape_cr = gdk_cairo_create(shape);
    clear_cairo_context(shape_cr); 
          
    /* This allows the mouse event to be passed to the window below when ungrab */
    gdk_window_input_shape_combine_mask (text_window->window, shape, 0, 0);  

    gdk_flush();

    cairo_destroy(shape_cr);
    g_object_unref(shape);

  #endif

  return TRUE;
}


/* Start the widget for the text insertion */
void start_text_widget(GtkWindow *parent, char* color, int tickness)
{
  text_pen_width = tickness;
  text_color = color;
  create_text_window(parent);
      
  gtk_widget_set_events (text_window, TEXT_MOUSE_EVENTS);

  g_signal_connect(G_OBJECT(text_window), "expose-event", G_CALLBACK(on_window_text_expose_event), NULL);
  g_signal_connect (G_OBJECT(text_window), "button_release_event", G_CALLBACK(release), NULL);
  g_signal_connect (G_OBJECT(text_window), "key_press_event", G_CALLBACK (key_press), NULL);

  gtk_widget_show_all(text_window);
  #ifdef _WIN32 
    // I use a layered window that use the black as transparent color
    setLayeredGdkWindowAttributes(text_window->window, RGB(0,0,0), 0, LWA_COLORKEY);	
  #endif
}


/* Stop the text insertion widget */
void stop_text_widget()
{
  if (text_cr)
    {
      if (letterlist)
        {
          g_slist_foreach(letterlist, (GFunc)g_free, NULL);
          g_slist_free(letterlist);
          letterlist = NULL;
          merge_context(text_cr);
        } 
      cairo_destroy(text_cr);     
      text_cr = NULL;
    }
  if (text_window)
    {
      gtk_widget_destroy(text_window);
      text_window= NULL;
    }
  if (text_cursor)
    {
      gdk_cursor_unref(text_cursor);
      text_cursor = NULL;
    }
  if (pos)
    {
      g_free(pos);
      pos = NULL;
    }
}


