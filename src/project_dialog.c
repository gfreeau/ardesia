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

#include <project_dialog.h>
#include <utils.h>


/*
 * Start the dialog that ask to the user
 * the project settings.
 */
gchar *
start_project_dialog (GtkWindow *parent)
{
  GtkWidget *project_dialog = NULL;
  GObject *project_obj = NULL;
  GObject *dialog_obj = NULL;
  GtkWidget *dialog_entry = NULL;
  gchar *ret = NULL;
  gint  pos = -1;
  ProjectData *project_data = (ProjectData *) g_malloc ( (gsize) sizeof (ProjectData));

  /* Initialize the main window. */
  project_data->project_dialog_gtk_builder = gtk_builder_new ();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file (project_data->project_dialog_gtk_builder, PROJECT_UI_FILE, NULL);

  /* Fill the window by the gtk builder xml */
  project_obj = gtk_builder_get_object (project_data->project_dialog_gtk_builder, "projectDialog");
  project_dialog = GTK_WIDGET (project_obj);
  gtk_window_set_transient_for (GTK_WINDOW (project_dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (project_dialog), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (project_dialog), TRUE);

#ifdef _WIN32
  /* 
   * In Windows the parent bar go above the dialog;
   * to avoid this behaviour I put the parent keep above to false.
   */
  gtk_window_set_keep_above (GTK_WINDOW (parent), FALSE);
#endif

  dialog_obj = gtk_builder_get_object (project_data->project_dialog_gtk_builder, "projectDialogEntry");
  dialog_entry = GTK_WIDGET (dialog_obj);

  project_data->project_name = g_strdup_printf ("ardesia_project_%s", get_date ());
  gtk_editable_insert_text (GTK_EDITABLE (dialog_entry), project_data->project_name, -1, &pos );

  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (project_data->project_dialog_gtk_builder, (gpointer) project_data);

  gtk_dialog_run (GTK_DIALOG (project_dialog));

  /* free the structures */
  g_object_unref (project_data->project_dialog_gtk_builder);
  project_data->project_dialog_gtk_builder = NULL;

  ret = g_strdup_printf ("%s", project_data->project_name);

  g_free (project_data->project_name);
  project_data->project_name = NULL;

  g_free (project_data);
  project_data = NULL;

  gtk_widget_destroy (project_dialog);
  project_dialog = NULL;
  return ret; 
}


