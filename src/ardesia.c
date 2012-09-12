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


#include <utils.h>
#include <annotation_window.h>
#include <background_window.h>
#include <project_dialog.h>
#include <share_confirmation_dialog.h>
#include <bar.h>

/*ch* External defined structure used to configure text input. (see text_window.c) */
#include <text_window.h>
extern TextConfig *text_config;

/* Print the version of the tool and exit. */
static void
print_version      ()
{
  g_printf ("Ardesia %s; the free digital sketchpad\n\n", PACKAGE_VERSION);
  exit (EXIT_SUCCESS);
}


/* Print the command line help. */
static void
print_help         ()
{
  gchar *year = "2009-2010";
  gchar *author = "Pietro Pilolli";
  g_printf ("Usage: %s [options] [filename]\n\n", PACKAGE_NAME);
  g_printf ("Ardesia the free digital sketchpad\n\n");
  g_printf ("options:\n");
  g_printf ("  --verbose ,\t-V\t\tEnable verbose mode to see the logs\n");
  g_printf ("  --decorate,\t-d\t\tDecorate the window with the borders\n");
  g_printf ("  --gravity ,\t-g\t\tSet the gravity of the bar. Possible values are:\n");
  g_printf ("  \t\t\t\teast [default]\n");
  g_printf ("  \t\t\t\twest\n");
  g_printf ("  \t\t\t\tnorth\n");
  g_printf ("  \t\t\t\tsouth\n");
  g_printf ("  --font ,\t-f\t\tSet the font family for the text window. Possible values are:\n");
  g_printf ("  \t\t\t\tserif [default]\n");
  g_printf ("  \t\t\t\tsans-serif\n");
  g_printf ("  \t\t\t\tmonospace\n");
  g_printf ("  --leftmargin,\t-l\t\tSet the left margin in text window to set after hitting Enter\n");
  g_printf ("  --tabsize,\t-t\t\tSet the tabsize in pixel in text window\n");
  g_printf ("  --help    ,\t-h\t\tShows the help screen\n");
  g_printf ("  --version ,\t-v\t\tShows version information and exit\n");
  g_printf ("\n");
  g_printf ("filename:\t  \t\tThe interactive Whiteboard Common File (iwb)\n");
  g_printf ("\n");
  g_printf ("%s (C) %s %s\n", PACKAGE_STRING, year, author);
  exit (EXIT_SUCCESS);
}


#ifndef _WIN32
/* Call the dialog that inform the user to enable a composite manager. */
static void
run_missing_composite_manager_dialog   ()
{
  GtkWidget *msg_dialog;
  msg_dialog = gtk_message_dialog_new (NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       gettext ("In order to run Ardesia you need to enable a composite manager"));

  gtk_dialog_run (GTK_DIALOG (msg_dialog));

  if (msg_dialog != NULL)
    {
      gtk_widget_destroy (msg_dialog);
      msg_dialog = NULL;
    }

  exit (EXIT_FAILURE);
}


/* Check if a composite manager is active. */
static void
check_composite_manager      ()
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen  *screen  = gdk_display_get_default_screen (display);
  gboolean composite = gdk_screen_is_composited (screen);

  if (!composite)
    {
      /* start the enable composite manager dialog. */
      run_missing_composite_manager_dialog ();
    }

}
#endif


/* Parse the command line in the standard getopt way. */
static CommandLine *
parse_options           (gint   argc,
                         char  *argv[])
{
  CommandLine *commandline = g_malloc ((gsize) sizeof (CommandLine));

  commandline->position = EAST;
  commandline->debug = FALSE;
  commandline->iwb_filename = NULL;
  commandline->decorated=FALSE;
  commandline->fontfamily = "serif";
  commandline->text_leftmargin = 0;
  commandline->text_tabsize = 80;

  /* Getopt_long stores the option index here. */
  while (1)
    {
      gint c;
      static struct option long_options[] =
      {/* These options set a flag. */
      {"help", no_argument,       0, 'h'},
      {"decorated", no_argument,  0, 'd'},
      {"verbose", no_argument,    0, 'V'},
      {"version", no_argument,    0, 'v'},
      /*
       * These options don't set a flag.
       * We distinguish them by their indices.
       */
      {"gravity", required_argument, 0, 'g'},
      {"font", required_argument, 0, 'f'},
      {"leftmargin", required_argument, 0, 'l'},
      {"tabsize", required_argument, 0, 't'},
      {0, 0, 0, 0}
      };

      gint option_index = 0;
      c = getopt_long (argc,
                       argv,
                       "hdvVg:f:l:t:",
                       long_options,
                       &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        {
          break;
        }

      switch (c)
        {
          case 'h':
            print_help ();
            break;
          case 'v':
            print_version ();
            break;
          case 'd':
            commandline->decorated=TRUE;
            break;
          case 'V':
            commandline->debug=TRUE;
            break;
          case 'g':
            if (g_strcmp0 (optarg, "east") == 0)
              {
                commandline->position = EAST;
              }
            else if (g_strcmp0 (optarg, "west") == 0)
              {
                commandline->position = WEST;
              }
            else if (g_strcmp0 (optarg, "north") == 0)
              {
                commandline->position = NORTH;
              }
            else if (g_strcmp0 (optarg, "south") == 0)
              {
                commandline->position = SOUTH;
              }
            else
              {
                print_help ();
              }
            break;
          case 'f':
            if (g_strcmp0 (optarg, "serif") == 0 ||
                g_strcmp0 (optarg, "sans-serif") == 0 ||
                g_strcmp0 (optarg, "monospace") == 0)
              {
                commandline->fontfamily = optarg;
              }
            break;
          case 'l':
            commandline->text_leftmargin = atoi(optarg);
            break;
          case 't':
            commandline->text_tabsize = atoi(optarg);
            break;
          default:
            print_help ();
            break;
        }
    }

  if (optind<argc)
    {
      commandline->iwb_filename = argv[optind];
    }

  return commandline;
}


/* Enable the localization support with gettext. */
static void
enable_localization_support       ()
{
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (GETTEXT_PACKAGE);
#endif
}


/* Create a shorcut to the workspace on the desktop. */
static void
create_workspace_shortcut (gchar *workspace_dir)
{
  gchar *desktop_entry_filename = g_strdup_printf ("%s%s%s_workspace",
                                                    get_desktop_dir (),
                                                    G_DIR_SEPARATOR_S,
                                                    PACKAGE_NAME);

#ifdef _WIN32
  windows_create_link (workspace_dir,
                       desktop_entry_filename,
                       "%SystemRoot%\\system32\\imageres.dll",
                       123);

#else
  xdg_create_link (workspace_dir , desktop_entry_filename, "folder-documents");
#endif
  g_free (desktop_entry_filename);
}


/* Configure the workspace. */
static gchar *
configure_workspace               (gchar *project_name)
{
  gchar *workspace_dir = (gchar *) NULL;
  const gchar *documents_dir = get_documents_dir ();

  /* The workspace directory is in the documents ardesia folder. */
  workspace_dir = g_build_filename (documents_dir,
                                    PACKAGE_NAME,
                                    (gchar *) 0);

  create_workspace_shortcut (workspace_dir);

  return workspace_dir;
}


/* Create the default project dir under the workspace_dir. */
static gchar *
create_default_project_dir        (gchar  *workspace_dir,
                                   gchar  *project_name)
{
  gchar *project_dir = g_build_filename (workspace_dir,
                                         project_name,
                                         (gchar *) 0);

  if (!file_exists (project_dir))
    {

      if (g_mkdir_with_parents (project_dir, 0700)==-1)
        {
          g_warning ("Unable to create folder %s\n", project_dir);
        }

    }

  return project_dir;
}


/* This is the starting point of the program. */
int
main                              (int    argc,
                                   char  *argv[])
{
  CommandLine *commandline = (CommandLine *) NULL;
  gchar       *project_name = (gchar *) NULL;
  gchar       *project_dir = (gchar *) NULL;
  gchar       *iwb_filename = (gchar *) NULL;
  GtkWidget   *background_window = (GtkWidget *) NULL;
  GtkWidget   *annotation_window = (GtkWidget *) NULL;
  GtkWidget   *ardesia_bar_window = (GtkWidget *) NULL;
  GSList      *artifact_list = (GSList *) NULL;

  /* Enable the localization support with gettext. */
  enable_localization_support ();

  gtk_init (&argc, &argv);

#ifndef _WIN32
  check_composite_manager ();
#endif

	
  /*
   * Uncomment this and the create_segmentation_fault function
   * to create a segmentation fault.
   */
  //create_segmentation_fault ();

  commandline = parse_options (argc, argv);
	
  /* Initialize new text configuration options. */
  text_config = g_malloc ((gsize) sizeof (TextConfig));
  text_config->fontfamily = commandline->fontfamily;
  text_config->leftmargin = commandline->text_leftmargin;
  text_config->tabsize = commandline->text_tabsize;

  if (commandline->iwb_filename)
    {
      gint init_pos = -1;
      gint end_pos = -1;


      if (g_path_is_absolute (commandline->iwb_filename))
        {
          iwb_filename = g_strdup (commandline->iwb_filename);
        }
      else
        {
          gchar *dir = g_get_current_dir ();
          iwb_filename = g_build_filename (dir, commandline->iwb_filename, (gchar *) 0);
          g_free (dir);
        }

      if (!file_exists(iwb_filename))
        {
          printf("No such file %s\n", iwb_filename);
          exit (EXIT_FAILURE);
        }

      init_pos = g_substrlastpos (iwb_filename, G_DIR_SEPARATOR_S);
      end_pos  = g_substrlastpos (iwb_filename, ".");
      project_name = g_substr (iwb_filename, init_pos+1, end_pos-1);
      project_dir = g_substr (iwb_filename, 0, init_pos-1);
    }
  else
    {
      gchar *workspace_dir = (gchar *) NULL;

      /* Show the project name wizard. */
      project_name = start_project_dialog ((GtkWindow *) NULL);
      workspace_dir = configure_workspace (project_name);
      project_dir = create_default_project_dir (workspace_dir, project_name);
      g_free (workspace_dir);
    }

  set_project_name (project_name);
  set_project_dir (project_dir);
  set_iwb_filename (iwb_filename);


  background_window = create_background_window ();

  if (background_window == NULL)
    {
      destroy_background_window ();
      g_free (commandline);
      exit (EXIT_FAILURE);
    }

  gtk_widget_show (background_window);
  
  set_background_window (background_window);
  
  /* Initialize the annotation window. */
  annotate_init (background_window, iwb_filename, commandline->debug);

  annotation_window = get_annotation_window ();

  if (annotation_window == NULL)
    {
      annotate_quit ();
      destroy_background_window ();
      g_free (commandline);
      exit (EXIT_FAILURE);
    }

  /* Postcondition: the annotation window is valid. */
  gtk_window_set_keep_above (GTK_WINDOW (annotation_window), TRUE);
  
  gtk_widget_show (annotation_window);
  
  ardesia_bar_window = create_bar_window (commandline, annotation_window);

  if (ardesia_bar_window == NULL)
    {
      annotate_quit ();
      destroy_background_window ();
      g_free (commandline);
      exit (EXIT_FAILURE);
    }

  gtk_window_set_keep_above (GTK_WINDOW (ardesia_bar_window), TRUE);
  gtk_widget_show (ardesia_bar_window);
  gtk_main ();

  artifact_list = get_artifacts ();

  if (artifact_list)
    {
      start_share_dialog ((GtkWindow *) NULL);
      free_artifacts ();
    }

  g_free (project_name);

  remove_dir_if_empty(project_dir);

  g_free (project_dir);
  
  g_free (iwb_filename);

  g_free (commandline);

  return 0;
}
