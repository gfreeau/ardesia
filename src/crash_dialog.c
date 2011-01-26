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



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <crash_dialog.h>
#include <utils.h>


/*
 * Start the dialog that ask to the user
 * if he wants crash you work
 */
void start_crash_dialog(GtkWindow *parent, gchar* crash_report)
{
  CrashData *crash_data = (CrashData *) g_malloc(sizeof(CrashData));

  GSList * artifact_list = get_artifacts();

  if (!artifact_list)
    {
       return;
    }  

  GtkWidget *crashDialog;

  /* Initialize the main window */
  crash_data->crashDialogGtkBuilder = gtk_builder_new();
  crash_data->crash_report = crash_report;

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(crash_data->crashDialogGtkBuilder, CRASH_UI_FILE, NULL);
 
  /* Fill the window by the gtk builder xml */
  crashDialog = GTK_WIDGET(gtk_builder_get_object(crash_data->crashDialogGtkBuilder,"crashDialog"));
  gtk_window_set_transient_for(GTK_WINDOW(crashDialog), parent);
  gtk_window_set_modal(GTK_WINDOW(crashDialog), TRUE);
  
#ifdef _WIN32
  /* 
   * In Windows the parent bar go above the dialog;
   * to avoid this behaviour I put the parent keep above to false
   */
  gtk_window_set_keep_above(GTK_WINDOW(parent), FALSE);
#endif 
  
  /* Connect all signals by reflection */
  gtk_builder_connect_signals (crash_data->crashDialogGtkBuilder, (gpointer) crash_data);

  gtk_dialog_run(GTK_DIALOG(crashDialog));

  gtk_widget_destroy(crashDialog);
}
