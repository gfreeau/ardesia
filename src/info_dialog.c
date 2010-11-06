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

#include <info_dialog.h>
#include <utils.h>
 

/*
 * Start the info dialog
 */
void start_info_dialog(GtkToolButton   *toolbutton, GtkWindow *parent)
{
  GtkWidget *infoDialog;

  /* Initialize the main window */
  GtkBuilder*  infoDialogGtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(infoDialogGtkBuilder, INFO_UI_FILE, NULL);

  /* Fill the window by the gtk builder xml */
  infoDialog = GTK_WIDGET(gtk_builder_get_object(infoDialogGtkBuilder,"aboutdialog"));
  gtk_window_set_transient_for(GTK_WINDOW(infoDialog), parent);
  gtk_window_set_modal(GTK_WINDOW(infoDialog), TRUE);

  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(infoDialog), VERSION);   

  /* Connect all signals by reflection */
  gtk_builder_connect_signals (infoDialogGtkBuilder, NULL);

  gtk_dialog_run(GTK_DIALOG(infoDialog));
  
  if (infoDialog != NULL)
    {
      gtk_widget_destroy(infoDialog);
    }

  g_object_unref (infoDialogGtkBuilder);
}


