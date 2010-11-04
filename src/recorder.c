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

#include <recorder.h>
#include <utils.h>


/* pid of the recording process */
static GPid recorderpid;

/* is the recorder active */
static gboolean is_active = FALSE;



/* 
 * Create a recorder process; it return the pid
 */
GPid call_recorder(gchar* filename, gchar* option)
{
  gchar* argv[4] = {RECORDER_FILE, option, filename, (gchar*) 0};
  GPid pid = (GPid) 0;
  if (
  g_spawn_async (NULL /*working_directory*/,
                 argv,
                 NULL /*envp*/,
                 G_SPAWN_SEARCH_PATH,
                 NULL /*child_setup*/,
                 NULL /*user_data*/,
                 &pid /*child_pid*/,
                 NULL /*error*/))
	{
           is_active = TRUE;	   
	}
  return pid;
}


/* Is the recorder available */
gboolean is_recorder_available()
{
  gchar* argv[5] = {"vlc", "-I", "dummy", "--dummy-quiet", (gchar*) 0};
  
  return g_spawn_async (NULL /*working_directory*/,
                        argv,
                        NULL /*envp*/,
                        G_SPAWN_SEARCH_PATH,
                        NULL /*child_setup*/,
                        NULL /*user_data*/,
                        NULL /*child_pid*/,
                        NULL /*error*/);
}


/* Return if the recording is active */
gboolean is_recording()
{
  return is_active;
}


/* Quit to record */
void quit_recorder()
{
  if (is_recording())
  {
    g_spawn_close_pid(recorderpid);
    recorderpid = call_recorder(NULL, "stop");
    g_spawn_close_pid(recorderpid);
    is_active = FALSE;
  }  
}


/* Missing program dialog */
void visualize_missing_recorder_program_dialog(GtkWindow* parent_window)
{
  GtkWidget *miss_dialog;
  
  miss_dialog = gtk_message_dialog_new (parent_window, GTK_DIALOG_MODAL, 
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK, 
                                        gettext("To record with Ardesia you must install the vlc program and add it to the PATH environment variable"));
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
gboolean start_save_video_dialog(GtkToolButton *toolbutton, GtkWindow *parent, gchar **workspace_dir)
{
  gboolean status = FALSE;

  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext("Save video as ogv"), parent, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);

  gtk_window_set_title (GTK_WINDOW (chooser), gettext("Choose a file"));
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), *workspace_dir);
  
  gchar* filename = get_default_name();

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      /* store the folder location this will be proposed the next time */
      g_free(*workspace_dir);
      *workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser)); 

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
     
      gchar* supported_extension = ".ogv";

      g_free(filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      gchar* filenamecopy = g_strdup_printf("%s",filename); 
      
      if (!g_str_has_suffix(filename, supported_extension))
        {
          g_free(filenamecopy);
          filenamecopy = g_strdup_printf("%s%s",filename,supported_extension);
        }      
      g_free(filename);   
      filename = filenamecopy;  
 
      if (file_exists(filename))
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
              gtk_widget_destroy (chooser);
	      return status; 
	    } 
	}
	 recorderpid = call_recorder(filename, "start");
	 status = (recorderpid > 0);
    }
  if (chooser)
   { 
     gtk_widget_destroy (chooser);
   } 
  g_free(filename);
  return status;
} 


