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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ardesia.h>

#include <config.h>

#include <gtk/gtk.h>
#include "stdlib.h"


/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif



#include "callbacks.h"
#include "annotate.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "getopt.h"
#include "utils.h"
#include "background.h"

#ifdef _WIN32
  #include <gdkwin32.h>
  #define UI_FILE "ardesia.glade"
  #define UI_HOR_FILE "ardesia_horizontal.glade"
#else
  #define UI_FILE PACKAGE_DATA_DIR"/ardesia/ui/ardesia.glade"
  #define UI_HOR_FILE PACKAGE_DATA_DIR"/ardesia/ui/ardesia_horizontal.glade"
#endif 


GtkBuilder *gtkBuilder;

int EAST=1;
int WEST=2;
int NORTH=3;
int SOUTH=4;


/* space from border in pixel */
int SPACE_FROM_BORDER = 25;

typedef struct
{
  char* backgroundimage;
  gboolean debug;
  gboolean decorated;
  int position;
} CommandLine;


CommandLine *commandline;

/* 
 * Calculate the position where calculate_initial_position the main window 
 * the centered position upon the window manager bar
 */
void calculate_position(GtkWidget *ardesia_bar_window, int dwidth, int dheight, int *x, int *y, int wwidth, int wheight, int position)
{
  *y = ((dheight - wheight)/2); 
  /* vertical layout */
  if (position==WEST)
    {
      *x = 0;
    }
  else if (position==EAST)
    {
      *x = dwidth - wwidth;
    }
  else
    {
      /* horizontal layout */
      *x = (dwidth - wwidth)/2;
      if (position==NORTH)
        {
          *y = SPACE_FROM_BORDER; 
        }
      else if (position==SOUTH)
        {
         /* south */
         *y = dheight - SPACE_FROM_BORDER - wheight;
        }
      else
        {  
          /* invalid position */
          perror ("Valid position are NORTH, SOUTH, WEST or EAST\n");
          exit(0);
        }
    }
}


/* 
 * Calculate initial position
 * Here can be beatiful have a configuration file
 * where put the user can decide the position of the window
 */
int calculate_initial_position(GtkWidget *ardesia_bar_window, int *x, int *y, int wwidth, int wheight, int position)
{
  gint dwidth = gdk_screen_width();
  gint dheight = gdk_screen_height();
  /* resize if larger that screen width */
  if (wwidth>dwidth)
    {
       wwidth = dwidth;
       gtk_window_resize(GTK_WINDOW(ardesia_bar_window), wwidth, wheight);       
    }
  /* resize if larger that screen height */
  if (wheight>dheight)
    {
       wheight = dheight - 15;
       gtk_widget_set_usize(ardesia_bar_window, wwidth, wheight);       
    }
  calculate_position(ardesia_bar_window, dwidth, dheight, x, y, wwidth, wheight, position);
  return 0; 
}


/* Print command line help */
void print_help()
{
  char* version = "0.3";
  char* year = "2009-2010";
  char* author = "Pietro Pilolli";
  printf("Usage: ardesia [options] [filename]\n\n");
  printf("options:\n");
  printf("  --verbose ,\t-v\t\tEnable verbose mode to see the logs\n");
  printf("  --decorate,\t-d\t\tDecorate the window with the borders\n");
  printf("  --gravity ,\t-g\t\tSet the gravity of the bar. Possible values are:\n");
  printf("  \t\t\t\teast [default]\n");
  printf("  \t\t\t\twest\n");
  printf("  \t\t\t\tnorth\n");
  printf("  \t\t\t\tsouth\n");
  printf("  --help    ,\t-h\t\tShows the help screen\n");
  printf("\n");
  printf("filename:\t  \t\tThe file containig the image to be be used as background\n");
  printf("\n");
  printf("Ardesia %s (C) %s %s\n", version, year, author);
  exit(1);
}


void run_missing_composite_manager_dialog()
{
   GtkWidget *msg_dialog; 
   msg_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, 
                                        GTK_BUTTONS_OK, gettext("In order to run Ardesia you need to enable the Compiz composite manager"));
                 
   gtk_dialog_run(GTK_DIALOG(msg_dialog));
   
   if (msg_dialog != NULL)
     {
       gtk_widget_destroy(msg_dialog);
     }
   exit(0);
}


void check_composite_manager(GdkScreen* screen)
{
  gboolean composite = gdk_screen_is_composited (screen);
  if (!composite)
    {
       /* start the dialog that says to enable the composite manager */
       run_missing_composite_manager_dialog();
    }
}


/* check if a composite manager is active */
void set_the_best_colormap()
{
    
    GdkDisplay *display = gdk_display_get_default ();
    GdkScreen   *screen = gdk_display_get_default_screen (display);
    #ifdef __FreeBSD__
       check_composite_manager(screen);
    #endif
    /* In linux operating system you must have a composite manager */
    #ifdef linux
       check_composite_manager(screen);
    #endif
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);
    if (colormap)
      {
        gtk_widget_set_default_colormap(colormap);
      }
}


CommandLine* parse_options(int argc, char *argv[])
{
  CommandLine* commandline = g_malloc(sizeof(CommandLine)); 
  commandline->position = EAST;
  commandline->debug = FALSE;
  commandline->backgroundimage = NULL;
  commandline->decorated=FALSE;

  /* getopt_long stores the option index here. */
  while (1)
    {
      int c;
      static struct option long_options[] =
	{
	  /* These options set a flag. */
	  {"help", no_argument,       0, 'h'},
          {"decorated", no_argument,  0, 'd'},
	  {"verbose", no_argument,       0, 'v'},
	  /* These options don't set a flag.
	     We distinguish them by their indices. */
	  {"gravity", required_argument, 0, 'g'},
	  {0, 0, 0, 0}
	};

      int option_index = 0;
      c = getopt_long(argc, argv, "hdvg:",
		      long_options, &option_index);
     
      /* Detect the end of the options. */
      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  print_help();
	  break;
        case 'd':
	  commandline->decorated=TRUE;
	  break;
	case 'v':
	  commandline->debug=TRUE;
	  break;
	case 'g':
	  if (strcmp (optarg, "east") == 0)
	    {
	      commandline->position = EAST;
	    }
	  else if (strcmp(optarg, "west") == 0)
	    {
	      commandline->position = WEST;
	    }
          else if (strcmp(optarg, "north") == 0)
	    {
	      commandline->position = NORTH;
	    }
          else if (strcmp(optarg, "south") == 0)
	    {
	      commandline->position = SOUTH;
	    }
	  else
	    {
	      print_help();
	    }
	  break;
	default:
	  print_help();
	  break;
	} 
    }
  if (optind<argc)
    {
      commandline->backgroundimage = argv[optind];
    } 
  return commandline;
}


BarData* init_bar_data()
{
  BarData *bar_data = (BarData *) g_malloc(sizeof(BarData));
  bar_data->color = g_malloc(COLORSIZE * sizeof (gchar));
  strcpy(bar_data->color, "FF0000FF");
  bar_data->color[8]=0;
  bar_data->annotation_is_visible = TRUE;
  bar_data->pencil = TRUE;
  bar_data->grab = TRUE;
  bar_data->text = FALSE;
  bar_data->thickness = 14;
  bar_data->highlighter = FALSE;
  bar_data->rectifier = FALSE;
  bar_data->rounder = FALSE;
  bar_data->arrow = FALSE;
  bar_data->workspace_dir = NULL;
  return bar_data;
}


GtkWidget*
create_bar_window (GtkWidget *parent)
{
  GtkWidget *bar_window;
  GError* error = NULL;

  gtkBuilder = gtk_builder_new();
  char* file = UI_FILE;
  if (commandline->position>2)
    {
      file = UI_HOR_FILE;
    }

  if (!gtk_builder_add_from_file (gtkBuilder, file, &error))
    {
      g_warning ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
    }  
  
  BarData *bar_data = init_bar_data();

  /* This is important; connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(gtkBuilder, (gpointer) bar_data);
  bar_window = GTK_WIDGET (gtk_builder_get_object(gtkBuilder, "winMain"));
  gtk_window_set_transient_for(GTK_WINDOW(bar_window), GTK_WINDOW(parent));
  if (commandline->decorated)
    {
       gtk_window_set_decorated(GTK_WINDOW(bar_window), TRUE);
    }

  return bar_window;
}


int
main (int argc, char *argv[])
{
  GtkWidget *ardesia_bar_window ;

  #ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
  #endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  set_the_best_colormap();

  commandline = parse_options(argc, argv);

  GtkWidget* background_window = create_background_window(commandline->backgroundimage); 
  gtk_window_set_keep_above(GTK_WINDOW(background_window), TRUE);
  gtk_widget_show (background_window);
  set_background_window(background_window);

  
  /** Init annotate */
  annotate_init(background_window, commandline->debug); 

  GtkWidget* annotation_window = get_annotation_window();  
  
  gtk_window_set_keep_above(GTK_WINDOW(annotation_window), TRUE);
  gtk_widget_show (annotation_window);
  

  ardesia_bar_window = create_bar_window(annotation_window);
  gtk_window_set_keep_above(GTK_WINDOW(ardesia_bar_window), TRUE);

  int width;
  int height;
  gtk_window_get_size(GTK_WINDOW(ardesia_bar_window) , &width, &height);

  int x, y;

  /* calculate_initial_position the window in the desired position */
  if (calculate_initial_position(ardesia_bar_window,&x,&y,width,height,commandline->position) < 0)
    {
      exit(0);
    }
  gtk_window_move(GTK_WINDOW(ardesia_bar_window),x,y);  
  
  gtk_widget_show(ardesia_bar_window);

  gtk_main();
  g_object_unref(gtkBuilder);	
  return 0;
}
