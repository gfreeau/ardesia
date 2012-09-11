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

#include <crash_dialog.h>
#include <utils.h>


/* Create a new crash data variable. */
static CrashData*
new_crash_data (gchar *crash_report)
{
  CrashData *crash_data = (CrashData *) g_malloc ((gsize) sizeof (CrashData));

  /* Initialize the data. */
  crash_data->crash_dialog_gtk_builder = gtk_builder_new ();
  crash_data->crash_report = crash_report;
  return crash_data;
}


/*
 * Start the dialog that ask to the user
 * if he wants crash you work.
 */
void start_crash_dialog (GtkWindow *parent,
                         gchar *crash_report)
{
  GtkWidget *crash_dialog = NULL;
  GObject *crash_obj = NULL;
  CrashData *crash_data = new_crash_data (crash_report);

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (crash_data->crash_dialog_gtk_builder, CRASH_UI_FILE, NULL);

  /* Fill the window by the gtk builder xml. */
  crash_obj = gtk_builder_get_object (crash_data->crash_dialog_gtk_builder, "CrashDialog");
  crash_dialog = GTK_WIDGET (crash_obj);
  gtk_window_set_transient_for (GTK_WINDOW (crash_dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (crash_dialog), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (crash_dialog), TRUE);

  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (crash_data->crash_dialog_gtk_builder, (gpointer) crash_data);

  gtk_dialog_run (GTK_DIALOG (crash_dialog));

  gtk_widget_destroy (crash_dialog);
}


