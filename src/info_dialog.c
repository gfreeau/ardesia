/* 
 * Ardesia -- a program for painting on the screen 
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 

#include "interface.h"
#include "utils.h"

/* Info dialog */
GtkBuilder*  infoDialogGtkBuilder = NULL;

/*
 * Start the dialog that give 
 * to the user the program'info
 */
void start_info_dialog(GtkToolButton   *toolbutton, GtkWindow *parent)
{
 char *installation_location = PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S;
 GtkWidget *infoDialog;

  /* Initialize the main window */
  infoDialogGtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gchar* name = "infoDialog.glade";
  gchar* ui_location =  (gchar *) g_malloc((strlen(installation_location) + strlen(name) + 1 )* sizeof(gchar));
  sprintf(ui_location, "%s%s", installation_location, name);
  gtk_builder_add_from_file(infoDialogGtkBuilder, ui_location, NULL);
  g_free(ui_location);

  /* Fill the window by the gtk builder xml */
  infoDialog = GTK_WIDGET(gtk_builder_get_object(infoDialogGtkBuilder,"aboutdialog"));
  gtk_window_set_transient_for(GTK_WINDOW(infoDialog), parent);
  gtk_window_stick((GtkWindow*)infoDialog);

  /* Connect all signals by reflection */
  gtk_builder_connect_signals (infoDialogGtkBuilder, NULL);

  gtk_dialog_run(GTK_DIALOG(infoDialog));
   if (infoDialog != NULL)
    {
      gtk_widget_destroy(infoDialog);
    }
  g_object_unref (infoDialogGtkBuilder);
}
