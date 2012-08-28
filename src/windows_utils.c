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


/* All this file will be build only on windows */
#ifdef _WIN32

#include <windows_utils.h>


BOOL (WINAPI *setLayeredWindowAttributesProc) (HWND hwnd,
                                               COLORREF cr_key,
                                               BYTE b_alpha,
                                               DWORD dw_flags) = NULL;
	


/* This is needed to wrap the setLayeredWindowAttributes throught the windows user32 dll. */
void
setLayeredGdkWindowAttributes (GdkWindow* gdk_window,
                               COLORREF cr_key,
                               BYTE b_alpha,
                               DWORD dw_flags)
{
  HWND hwnd = GDK_WINDOW_HWND (gdk_window);
  HINSTANCE h_instance = LoadLibraryA ("user32");		

  setLayeredWindowAttributesProc = (BOOL (WINAPI*) (HWND hwnd,
                                                    COLORREF cr_key,
                                                    BYTE b_alpha,
                                                    DWORD dw_flags))
  GetProcAddress (h_instance, "SetLayeredWindowAttributes");

  setLayeredWindowAttributesProc (hwnd, cr_key, b_alpha, dw_flags);
}


/* Send an email with MAPI. */
void
windows_send_email (gchar  *to,
                    gchar  *subject,
                    gchar  *body,
                    GSList *attachment_list)
{
  HINSTANCE inst;
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
      gchar *attachment =  g_slist_nth_data (attachment_list, i);
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
                     int    icon_index)
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


