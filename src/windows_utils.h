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


#ifdef _WIN32

#include <windows.h>
#include <mapi.h>

#include <winuser.h>
#include <shlwapi.h>
#include <objbase.h>

#include <gdk/gdk.h>
#include <gdkwin32.h>
#include <gtk/gtk.h>


  
/* Define other symbols needed to create the transparent layered window */
#define LWA_COLORKEY	0x00000001


/* Set layered window attributes to a gdk window */
void
setLayeredGdkWindowAttributes (GdkWindow *gdk_window,
                               COLORREF   cr_key,
                               BYTE       b_alpha,
                               DWORD dw_flags);


/* Send an email with MAPI. */
void
windows_send_email (gchar  *to,
                    gchar  *subject,
                    gchar  *body,
                    GSList *attachment_list);


/* Create a link with icon. */
void
windows_create_link (gchar *src,
                     gchar *dest,
                     gchar *icon_path,
                     int    icon_index);

#endif


