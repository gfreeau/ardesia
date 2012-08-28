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


#include <utils.h>
#include <crash_dialog.h>


/* Shot when the ok button in crash dialog is pushed. */
G_MODULE_EXPORT void
on_crash_ok_button_clicked (GtkButton *buton,
                            gpointer data)
{
  CrashData *crash_data = (CrashData *) data;
  send_trace_with_email (crash_data->crash_report);
}


/* Shot when the ok button in preference dialog is pushed. */
G_MODULE_EXPORT void
on_crash_cancel_button_clicked (GtkButton *buton,
                                gpointer data)
{
  /* Do nothing. */
}


