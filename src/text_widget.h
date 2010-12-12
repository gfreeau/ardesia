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

/** Widget for text insertion */

#include <ctype.h>

#include <gtk/gtk.h>

#include <cairo.h>


#ifdef _WIN32
#include <cairo-win32.h>
#else
#ifdef __APPLE__
#include <cairo-quartz.h>
#else
#include <cairo-xlib.h>
#endif
#endif


#define TEXT_CURSOR_WIDTH 4

#ifdef _linux
#define VIRTUALKEYBOARD_NAME "florence"
#endif

#ifdef _WIN32
#define VIRTUALKEYBOARD_NAME "C:\\Program Files\\Common Files\\Microsoft Shared\\ink\\TabTip.exe"
#define VIRTUALKEYBOARD_WINDOW_NAME "IPTip_Main_Window" 
 
#define TEXT_UI_FILE "..\\share\\ardesia\\ui\\text.glade"
#define TEXT_MOUSE_EVENTS        ( GDK_POINTER_MOTION_MASK |    \
				   GDK_BUTTON_PRESS_MASK |      \
                                   GDK_BUTTON_RELEASE_MASK      \
                                   )


#else
#define TEXT_UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/text.glade"
#endif 



typedef struct
{
  gdouble x;
  gdouble y;
  gdouble x_bearing;
  gdouble y_bearing;
} CharInfo;


typedef struct
{
  gdouble x;
  gdouble y;
} Pos;


typedef struct
{
  /* Gtkbuilder to build the window */
  GtkBuilder *gtk_builder;
  GtkWidget* window;
  GPid virtual_keyboard_pid;
  cairo_t *cr;
  Pos* pos;
  GSList *letterlist; 
  gchar* color;
  gint pen_width;
  gdouble max_font_height;
  cairo_text_extents_t extents;
  gint timer;
  gboolean blink_show;
  guint snooper_handler_id;
}TextData;


/* Start text widget */
void start_text_widget(GtkWindow *parent, gchar* color, gint tickness);


/* Stop text widget */
void stop_text_widget();


