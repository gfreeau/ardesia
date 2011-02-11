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


/* All this file will be build only on windows */
#ifdef _WIN32

#include <windows_utils.h>

 
BOOL (WINAPI *setLayeredWindowAttributesProc) (HWND hwnd, COLORREF cr_key,
					       BYTE b_alpha, DWORD dw_flags) = NULL;
	


/* This is needed to wrap the setLayeredWindowAttributes throught the windows user32 dll. */
void
setLayeredGdkWindowAttributes (GdkWindow* gdk_window,
			       COLORREF cr_key,
			       BYTE b_alpha,
			       DWORD dw_flags)
{
  HWND hwnd = GDK_WINDOW_HWND (gdk_window);
  h_instance h_instance = LoadLibraryA ("user32");		

  setLayeredWindowAttributesProc = (BOOL (WINAPI*) (HWND hwnd,
						    COLORREF cr_key,
						    BYTE b_alpha,
						    DWORD dw_flags))
    GetProcAddress (h_instance,"SetLayeredWindowAttributes");

  setLayeredWindowAttributesProc (hwnd, cr_key, b_alpha, dw_flags);
}


/* Is the two color similar. */
static gboolean
colors_too_similar (const GdkColor *color_a,
		    const GdkColor *color_b)
{
  return (abs (color_a->red - color_b->red) < 256 &&
	  abs (color_a->green - color_b->green) < 256 &&
	  abs (color_a->blue - color_b->blue) < 256);
}


/* 
 * The function gdk_cursor_new_from_pixmap is broken on Windows.
 * this is a workaround using gdk_cursor_new_from_pixbuf. 
 * Thanks to Dirk Gerrits for this contribution .
 */
GdkCursor*
fixed_gdk_cursor_new_from_pixmap (GdkPixmap *source,
				  GdkPixmap *mask,
				  const GdkColor *fg,
				  const GdkColor *bg,
				  gint x,
				  gint y)
{
  GdkPixmap *rgb_pixmap;
  GdkGC *gc;
  GdkPixbuf *rgb_pixbuf, *rgba_pixbuf;
  GdkCursor *cursor;
  gint width, height;

  /*
   * @HACK  It seems impossible to work with RGBA pixmaps directly in
   * GDK-Win32.  Instead we pick some third color, different from fg
   * and bg, and use that as the transparent colour'.
   * We do this using colors_too_similar (see above) because two colors could be
   * unequal in GdkColor's 16-bit/sample, but equal in GdkPixbuf's
   * 8-bit/sample. 
   *
   */
  GdkColor candidates[3] = {{0,65535,0,0}, {0,0,65535,0}, {0,0,0,65535}};
  GdkColor *trans = &candidates[0];
  if (colors_too_similar (trans, fg) || colors_too_similar (trans, bg)) 
    {
      trans = &candidates[1];
      if (colors_too_similar (trans, fg) || colors_too_similar (trans, bg))
        {
          trans = &candidates[2];
        }
    }
  /* Trans is now guaranteed to be unique from fg and bg. */

  /* Create an empty pixmap to hold the cursor image. */
  gdk_drawable_get_size (source, &width, &height);
  rgb_pixmap = gdk_pixmap_new (NULL, width, height, 24);

  /* Blit the bitmaps defining the cursor onto a transparent background. */
  gc = gdk_gc_new (rgb_pixmap);
  gdk_gc_set_fill (gc, GDK_SOLID);
  gdk_gc_set_rgb_fg_color (gc, trans);
  gdk_draw_rectangle (rgb_pixmap, gc, TRUE, 0, 0, width, height);
  gdk_gc_set_fill (gc, GDK_OPAQUE_STIPPLED);
  gdk_gc_set_stipple (gc, source);
  gdk_gc_set_clip_mask (gc, mask);
  gdk_gc_set_rgb_fg_color (gc, fg);
  gdk_gc_set_rgb_bg_color (gc, bg);
  gdk_draw_rectangle (rgb_pixmap, gc, TRUE, 0, 0, width, height);
  gdk_gc_unref (gc);

  /* Create a cursor out of the created pixmap. */
  rgb_pixbuf = gdk_pixbuf_get_from_drawable (
					     NULL,
					     rgb_pixmap,
					     gdk_colormap_get_system (),
					     0,
					     0,
					     0,
					     0,
					     width,
					     height);

  gdk_pixmap_unref (rgb_pixmap);
  rgba_pixbuf = gdk_pixbuf_add_alpha (rgb_pixbuf, TRUE, trans->red, trans->green, trans->blue);
  gdk_pixbuf_unref (rgb_pixbuf);
  cursor = gdk_cursor_new_from_pixbuf (gdk_display_get_default (), rgba_pixbuf, x, y);
  gdk_pixbuf_unref (rgba_pixbuf);

  return cursor;
}


/* Send an email with MAPI. */
void
windows_send_email (gchar *to,
		    gchar *subject,
		    gchar *body,
		    GSList *attachment_list)
{
  h_instance inst;
  LPMAPISENDMAIL MAPISendMail;

  inst = LoadLibrary ("MAPI32.DLL");

  MAPISendMail = (LPMAPISENDMAIL) GetProcAddress (inst, "MAPISendMail");

  MapiMessage m_msg;
  ZeroMemory (&m_msg, sizeof (m_msg));

  m_msg.ulReserved = 0;
  m_msg.lpszSubject = subject;
  m_msg.lpszNoteText = body;
  m_msg.lpszMessageType = NULL;
  m_msg.lpszDateReceived = NULL;
  m_msg.lpszConversationID = NULL;
  m_msg.flFlags = 0;
  m_msg.lpOriginator = NULL;

  MapiRecipDesc m_rd[1];
  ZeroMemory (&m_rd, sizeof (m_rd));

  m_rd[0].ulRecipClass = MAPI_TO;
  m_rd[0].lpszName = to;

  m_msg.nRecipCount = sizeof (m_rd) / sizeof (m_rd[0]);
  m_msg.lpRecips = m_rd;

  gint attach_lenght = g_slist_length (attachment_list);
  
  MapiFileDesc m_fd[attach_lenght];
  
  gint i =0;
  for (i=0; i<attach_lenght; i++)
    { 
      gchar* attachment =  g_slist_nth_data (attachment_list, i);
      m_fd[i].lpszPathName = attachment;
      m_fd[i].lpszFileName = attachment;
    }  

  m_msg.nFileCount = sizeof (m_fd) / sizeof (m_fd[0]);
  m_msg.lpFiles = m_fd;

  MAPISendMail (0, 0, &m_msg, MAPI_LOGON_UI | MAPI_DIALOG, 0L);
}


/* Create a link with icon. */
void
windows_create_link (gchar *src,
		     gchar *dest,
		     gchar *icon_path,
		     int icon_index)
{  

  gchar *extension = "lnk";
  gchar *link_filename = g_strdup_printf ("%s.%s", dest, extension);
  if (g_file_test (link_filename, G_FILE_TEST_EXISTS))
    {
      g_free (link_filename);
      return;
    }

  IShellLink *shell_link;
  IPersistFile *persist_file;
  WCHAR wsz[MAX_PATH];
  
  CoCreateInstance (&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID *) &shell_link);

  shell_link->lpVtbl->SetPath (shell_link, (LPCTSTR) src);
  shell_link->lpVtbl->SetIconLocation (shell_link, (LPCTSTR) icon_path, icon_index);

  shell_link->lpVtbl->QueryInterface (shell_link, &IID_IPersistFile, (LPVOID *)&persist_file);

  MultiByteToWideChar (CP_ACP, 0, (PTSTR) link_filename, -1, wsz, MAX_PATH);
  g_free (link_filename);

  persist_file->lpVtbl->Save (persist_file, wsz, TRUE);
  persist_file->lpVtbl->Release (persist_file);
  shell_link->lpVtbl->Release (shell_link);
}

#endif


