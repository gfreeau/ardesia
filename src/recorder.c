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
 * ardesia is distributed in the hope that it will be useful, but
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

#include <gtk/gtk.h>
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include "utils.h"
#ifdef _WIN32
  #include <gdkwin32.h>
  #include <windows.h>
#else
  #include <sys/wait.h>
#endif

#ifndef _WIN32
  #define RECORDER_FILE PACKAGE_DATA_DIR"/ardesia/scripts/recordmydesktop_screencast.sh"
#endif 


/* pid of the recording process */
int          recorderpid = -1;
gchar* filename = NULL;

/* 
 * Create a annotate client process the annotate
 * that talk with the server process option is start or stop
 */
int call_recorder(char* option)
{
  #ifdef _WIN32
    //TODO: this feature is not yet implemented on win32
    return -1;
  #else
    char* argv[4];
    argv[0] = RECORDER_FILE;
    argv[1] = filename;
    argv[2] = option;
    argv[3] = (char*) NULL ;
  
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
  #endif
}


int check_recorder()
{
  #ifdef _WIN32
    //TODO: this feature is not yet implemented on win32
    return -1;
  #else
    char* argv[3];
    argv[0] = "recordmydesktop";
    argv[1] = "--version";
    argv[2] = (char*) NULL ;
  
    pid_t pid;
    pid = fork();
    int status;
    wait(&status);
    if (pid == 0)
      {
        if (execvp(argv[0], argv) < 0)
          {
            return -1;
          }
      }
    return pid;
  #endif
}


/* Return if the recording is active */
gboolean is_recording()
{
  #ifdef _WIN32
    //TODO: this feature is not yet implemented on win32
    return FALSE;
  #else
    if (recorderpid == -1)
    {
      return FALSE;
    }
    return TRUE;
  #endif
}

/* Quit to record */
void quit_recorder()
{
  #ifdef _WIN32
    //TODO
  #else
    if(is_recording())
      {
        kill(recorderpid,SIGTERM);
        recorderpid=-1;
        call_recorder("stop");
        g_free(filename);
      }  
  #endif
}


/* Missing program dialog */
void missing_recorder_program_dialog(GtkWindow* parent_window)
{
  GtkWidget *miss_dialog;
  
  miss_dialog = gtk_message_dialog_new (parent_window, GTK_DIALOG_MODAL, 
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK, 
                                        gettext("To record with Ardesia you must install the recordmydesktop program"));
  gtk_window_set_modal(GTK_WINDOW(miss_dialog), TRUE);
 
  gtk_dialog_run(GTK_DIALOG(miss_dialog));
  
  if (miss_dialog != NULL)
    {
      gtk_widget_destroy(miss_dialog);
    }
}


/*
 * Start the dialog that ask to the user where save the video
 * containing the screencast
 * This function take as input the recor toolbutton in ardesia bar
 * return true is the recorder is started
 */
gboolean start_save_video_dialog(GtkToolButton *toolbutton, GtkWindow *parent, char **workspace_dir)
{
  gboolean status = FALSE;
   
  char * date = get_date();
  if (!(*workspace_dir))
    {
      *workspace_dir = (char *) get_desktop_dir();
    }	

  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save video as ogv"), parent, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);


  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Select a file"));
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);
  filename =  g_malloc(256*sizeof(gchar));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
     
      gchar* tmp = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
      strcpy(*workspace_dir, tmp);
      g_free(tmp);
      char* supported_extension = ".ogv";
      char* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, supported_extension) != 0))
	{
	  filename = (gchar *) g_realloc(filename,  (strlen(filename) + strlen(supported_extension) + 1) * sizeof(gchar));
	  (void) strcat((gchar *)filename, supported_extension);
          g_free(extension);
	}
 
      if (file_exists(filename, (char *) workspace_dir))
	{
	  GtkWidget *msg_dialog; 
                   
	  msg_dialog = gtk_message_dialog_new (GTK_WINDOW(chooser), 
					       GTK_DIALOG_MODAL, 
                                               GTK_MESSAGE_WARNING,  
                                               GTK_BUTTONS_YES_NO, gettext("File Exists. Overwrite"));
	     
          gint result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog != NULL)
            {
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result  == GTK_RESPONSE_NO)
	    { 
	      g_free(filename);
              g_free(date);
              gtk_widget_destroy (chooser);
	      return status; 
	    } 
	}
      recorderpid = check_recorder();
      if (recorderpid > 0)
        {
          status = TRUE;
          recorderpid = call_recorder("start");
        }
      else
       {
         status = FALSE;
         missing_recorder_program_dialog(parent); 
         gtk_widget_hide(GTK_WIDGET(toolbutton));
       }
    }
  if (chooser)
   { 
     gtk_widget_destroy (chooser);
   } 
      
  g_free(date);
  return status;
} 
