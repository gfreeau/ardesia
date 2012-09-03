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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <utils.h>
#include <keyboard.h>


/* The pid of the virtual keyboard process. */
static GPid virtual_keyboard_pid;


/* Start the virtual keyboard. */
void
start_virtual_keyboard       ()
{
  gchar *argv[2] = {VIRTUALKEYBOARD_NAME, (gchar *) 0};

  g_spawn_async (NULL /*working_directory*/,
                 argv,
                 NULL /*envp*/,
                 G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                 NULL /*child_setup*/,
                 NULL /*user_data*/,
                 &virtual_keyboard_pid /*child_pid*/,
                 NULL /*error*/);
}


/* Stop the virtual keyboard. */
void
stop_virtual_keyboard        ()
{
  if (virtual_keyboard_pid > 0)
    {
      /* @TODO replace this with the cross platform g_pid_terminate
       * when it will available
       */
#ifdef _WIN32
      HWND hwnd = FindWindow (VIRTUALKEYBOARD_WINDOW_NAME, NULL);
      SendMessage (hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
      kill (virtual_keyboard_pid, SIGTERM);
#endif
      g_spawn_close_pid (virtual_keyboard_pid);
      virtual_keyboard_pid = (GPid) 0;
    }
}


