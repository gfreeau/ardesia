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
#include <stdlib.h>

/* The pid of the virtual keyboard process. */
static GPid virtual_keyboard_pid;


/* Start the virtual keyboard. */
void
start_virtual_keyboard       ()
{
  int result = -1;
  result = system("gsettings set org.florence.behaviour auto-hide false");
  if (result != 0)
    {
      g_printerr ("Fail to show virtual keyboard: Florence and gsettings packages are required\n");
    }
}


/* Stop the virtual keyboard. */
void
stop_virtual_keyboard        ()
{
#ifdef _WIN32
  if (virtual_keyboard_pid > 0)
    {
      /* @TODO replace this with the cross platform g_pid_terminate
       * when it will available
       */
      HWND hwnd = FindWindow (VIRTUALKEYBOARD_WINDOW_NAME, NULL);
      SendMessage (hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
      g_spawn_close_pid (virtual_keyboard_pid);
      virtual_keyboard_pid = (GPid) 0;
    }
#else
  int result = -1;
  result = system("gsettings set org.florence.behaviour auto-hide true");
  if (result != 0)
    {
      g_printerr ("Fail to hide virtual keyboard: Florence and gsettings packages are required\n");
    }
#endif
}


