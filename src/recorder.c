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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include "utils.h"
#include "gettext.h"

/* pid of the recording process */
int          recorderpid = -1;

/* 
 * Create a annotate client process the annotate
 * that talk with the server process 
 */
int start_recorder(char* filename)
{
  char* argv[5];
  argv[0] = "recordmydesktop";
  argv[1] = "--on-the-fly-encoding";
  argv[2] = "-o";
  argv[3] = filename;
  argv[4] = (char*) NULL ;
  pid_t pid;

  pid = fork();

  if (pid < 0)
    {
      return -1;
    }
  if (pid == 0)
    {
      if (execvp(argv[0], argv) < 0)
        {
          return -1;
        }
    }
  return pid;
}

/* Return if the recording is active */
gboolean is_recording()
{
  if (recorderpid == -1)
    {
      return FALSE;
    }
  return TRUE;
}

/* Quit to record */
void quit_recorder()
{
  if(is_recording())
    {
      kill(recorderpid,SIGTERM);
      recorderpid=-1;
    }  
}


/* Missing program dialog */
void missing_program_dialog()
{
  GtkWidget *msg_dialog;
  msg_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK, gettext("To record with Ardesia you must install the recordmydesktop program"));
  gtk_window_stick((GtkWindow*)msg_dialog);

  gtk_dialog_run(GTK_DIALOG(msg_dialog));
  if (msg_dialog != NULL)
   {
     gtk_widget_destroy(msg_dialog);
   }
}


/*
 * Start the dialog that ask to the user where save the video
 * containing the screencast
 * This function take as input the recor toolbutton in ardesia bar
 * return true is the recorder is started
 */
gboolean start_save_video_dialog(GtkToolButton   *toolbutton, GtkWindow *parent, char *workspace_dir)
{
  gboolean status = FALSE;
   
  char * date = get_date();
  if (workspace_dir == NULL)
    {
      workspace_dir = (char *) get_desktop_dir();
    }	

  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save video as ogv"), parent, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  gtk_window_stick((GtkWindow*)chooser);
  
  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Select a file"));
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), workspace_dir);
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
      char* supported_extension = ".ogv";
      char* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, supported_extension) != 0))
	{
	  filename = (gchar *) realloc(filename,  (strlen(filename) + strlen(supported_extension) + 1) * sizeof(gchar));
	  (void) strcat((gchar *)filename, supported_extension);
          free(extension);
	}
 
      if (file_exists(filename, (char *) workspace_dir) == TRUE)
	{
	  GtkWidget *msg_dialog; 
                   
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,  GTK_BUTTONS_YES_NO, gettext("File Exists. Overwrite"));

	  gtk_window_stick((GtkWindow*)msg_dialog);
                 
          gint result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog != NULL)
            {
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result  == GTK_RESPONSE_NO)
	    { 
	      g_free(filename);
	      return status; 
	    } 
	}
      recorderpid = start_recorder(filename);
      if (recorderpid > 0)
        {
          status = TRUE;
        }
      else
       {
         status = FALSE;
         missing_program_dialog(); 
       }
    }
    if (chooser != NULL)
      { 
        gtk_widget_destroy (chooser);
      } 
      
  g_free(filename);
  g_free(date);
  return status;
} 




