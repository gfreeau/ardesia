/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
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

/*
 * File to load the interface of the program
 * this  use the GtkBuilder xml format of Glade-3
 * to define the gui interface
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "interface.h"


/* Set the defult width of the pen line */ 
void setInitialWidth(int val)
{
  GtkWidget* widthWidget = GTK_WIDGET(gtk_builder_get_object(gtkBuilder,"thickScale"));
  GtkHScale* hScale = (GtkHScale *) widthWidget;
  gtk_range_set_value(&hScale->scale.range, val);
}


/* Create the main window */
GtkWidget* create_mainWindow (void)
{
  GtkWidget *main_window = NULL;
  gtkBuilder = NULL;
  
  /* Initialize the main window */
  gtkBuilder = gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(gtkBuilder, PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S PACKAGE".glade", NULL);
  
  /* Fill the window by the gtk builder xml */
  main_window = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "winMain"));

  /* Set the width to 15 in the thick scale */ 
  setInitialWidth(15);
  
  /* Connect all signals by reflection */
  gtk_builder_connect_signals ( gtkBuilder, NULL );
  
  return main_window;
}
