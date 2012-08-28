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
#include <project_dialog.h>
#include <keyboard.h>


/* This shot when the text entry is in focus. */
G_MODULE_EXPORT gboolean 
on_project_dialog_entry_focus_in_event (GtkWidget *widget,
                                        GdkEvent  *event,
                                        gpointer   func_data)
{
  start_virtual_keyboard ();
  return TRUE;
}


/* Shot when the ok button in preference dialog is pushed. */
G_MODULE_EXPORT void
on_project_ok_button_clicked (GtkButton *buton,
                              gpointer   data)
{
  ProjectData *project_data = (ProjectData *) data;
  GObject *dialog_obj = gtk_builder_get_object (project_data->project_dialog_gtk_builder,
                                                "projectDialogEntry");

  GtkWidget *dialog_entry = GTK_WIDGET (dialog_obj);
  gchar *str = gtk_editable_get_chars (GTK_EDITABLE (dialog_entry), 0, -1);

  g_free (project_data->project_name); 
  project_data->project_name = str;
  stop_virtual_keyboard (); 
}


