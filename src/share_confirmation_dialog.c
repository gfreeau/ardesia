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

#include <share_confirmation_dialog.h>
#include <utils.h>


/*
 * Start the dialog that ask to the user
 * if he wants share his work.
 */
void
start_share_dialog                (GtkWindow *parent)
{
  GSList *artifact_list = get_artifacts ();

  if (!artifact_list)
    {
      return;
    }
  else
    {
      GtkWidget *share_dialog = (GtkWidget *) NULL;
      /* Initialize the main window. */
      GtkBuilder *share_dialog_gtk_builder = gtk_builder_new ();

      /* Load the gtk builder file created with glade. */
      gtk_builder_add_from_file (share_dialog_gtk_builder, SHARE_UI_FILE, NULL);

      /* Fill the window by the gtk builder xml. */
      share_dialog = GTK_WIDGET (gtk_builder_get_object (share_dialog_gtk_builder,
                                                         "shareDialog"));

      gtk_window_set_transient_for (GTK_WINDOW (share_dialog), parent);
      gtk_window_set_modal (GTK_WINDOW (share_dialog), TRUE);
      //gtk_window_set_keep_above (GTK_WINDOW (share_dialog), TRUE);

      /* Connect all signals by reflection. */
      gtk_builder_connect_signals (share_dialog_gtk_builder, (gpointer) NULL);

      gtk_dialog_run (GTK_DIALOG (share_dialog));

      gtk_widget_destroy (share_dialog);
      share_dialog = (GtkWidget *) NULL;
    }

}
