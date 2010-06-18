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
  #include <gdkwin32.h>
  #include <winuser.h>  
  BOOL (WINAPI *setLayeredWindowAttributesTextProc) (HWND hwnd, COLORREF crKey,
	BYTE bAlpha, DWORD dwFlags) = NULL;
#else
  #ifdef __APPLE__
    #include <cairo-quartz.h>
  #else
    #include <cairo-xlib.h>
  #endif
#endif


#define TEXT_MOUSE_EVENTS        ( GDK_POINTER_MOTION_MASK |	\
				   GDK_BUTTON_RELEASE_MASK      \
				 )

typedef struct
{
  int x;
  int y;
  int x_bearing;
  int y_bearing;
} CharInfo;


typedef struct
{
  int x;
  int y;
} Pos;


/* Start text widget */
void start_text_widget(GtkWindow *parent, char* color, int tickness);

/* Stop text widget */
void stop_text_widget();
