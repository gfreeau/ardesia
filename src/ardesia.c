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

#include <ardesia.h>
#include <utils.h>
#include <annotate.h>
#include <background.h>
#include <bar_callbacks.h>


#ifdef linux
/* globals retained across calls to resolve. */
static bfd* abfd = 0;
static asymbol **syms = 0;
static asection *text = 0;
#endif


/* 
 * Calculate the better position where put the bar
 */
void calculate_position(GtkWidget *ardesia_bar_window, 
                        gint dwidth, gint dheight, 
                        gint *x, gint *y, 
                        gint wwidth, gint wheight,
                        gint position)
{
  *y = ((dheight - wheight)/2);
  /* vertical layout */
  if (position == WEST)
    {
      *x = 0;
    }
  else if (position == EAST)
    {
      *x = dwidth;
    }
  else
    {
      /* horizontal layout */
      *x = (dwidth - wwidth)/2;
      if (position == NORTH)
        {
          *y = 0; 
        }
      else if (position == SOUTH)
        {
	  /* south */
	  *y = dheight;
        }
      else
        {  
          /* invalid position */
          perror("Valid position are NORTH, SOUTH, WEST or EAST\n");
          exit(0);
        }
    }
}


/* 
 * Calculate teh initial position
 * @TODO Here can be beatiful have a configuration file
 * where put the user can decide the position of the window
 */
void calculate_initial_position(GtkWidget *ardesia_bar_window, 
                                gint *x, gint *y, 
                                gint wwidth, gint wheight, 
                                gint position)
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
      gint tollerance = 15;
      wheight = dheight - tollerance;
      gtk_widget_set_usize(ardesia_bar_window, wwidth, wheight);       
    }

  calculate_position(ardesia_bar_window, dwidth, dheight, x, y, wwidth, wheight, position);
}


/* Print the command line help */
void print_help()
{
  gchar* version = "0.5";
  gchar* year = "2009-2010";
  gchar* author = "Pietro Pilolli";
  g_printf("Usage: ardesia [options] [filename]\n\n");
  g_printf("options:\n");
  g_printf("  --verbose ,\t-v\t\tEnable verbose mode to see the logs\n");
  g_printf("  --decorate,\t-d\t\tDecorate the window with the borders\n");
  g_printf("  --gravity ,\t-g\t\tSet the gravity of the bar. Possible values are:\n");
  g_printf("  \t\t\t\teast [default]\n");
  g_printf("  \t\t\t\twest\n");
  g_printf("  \t\t\t\tnorth\n");
  g_printf("  \t\t\t\tsouth\n");
  g_printf("  --help    ,\t-h\t\tShows the help screen\n");
  g_printf("\n");
  g_printf("filename:\t  \t\tThe file containig the image to be be used as background\n");
  g_printf("\n");
  g_printf("Ardesia %s (C) %s %s\n", version, year, author);
  exit(1);
}


/* Call the dialog that inform the user to enable a composite manager */
void run_missing_composite_manager_dialog()
{
  GtkWidget *msg_dialog; 
  msg_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, 
				      GTK_BUTTONS_OK, 
				      gettext("In order to run Ardesia you need to enable a composite manager")
                                      );
                 
  gtk_dialog_run(GTK_DIALOG(msg_dialog));
   
  if (msg_dialog != NULL)
    {
      gtk_widget_destroy(msg_dialog);
    }
  exit(0);
}


/* Check if a composite manager is active */
void check_composite_manager(GdkScreen* screen)
{
  gboolean composite = gdk_screen_is_composited(screen);
  if (!composite)
    {
      /* start the enable composite manager dialog */
      run_missing_composite_manager_dialog();
    }
}


/* Set the best colormap available on the system */
void set_the_best_colormap()
{
  GdkDisplay *display = gdk_display_get_default();
  GdkScreen   *screen = gdk_display_get_default_screen(display);

  /* In FreeBSD operating system you might have a composite manager */
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
      gdk_colormap_unref(colormap);
    }
}


/* Parse the command line in the standar getopt way */
CommandLine* parse_options(gint argc, char *argv[])
{
  CommandLine* commandline = g_malloc(sizeof(CommandLine)); 
  
  commandline->position = EAST;
  commandline->debug = FALSE;
  commandline->backgroundimage = NULL;
  commandline->decorated=FALSE;

  /* getopt_long stores the option index here. */
  while (1)
    {
      gint c;
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

      gint option_index = 0;
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
	  if (g_strcmp0(optarg, "east") == 0)
	    {
	      commandline->position = EAST;
	    }
	  else if (g_strcmp0(optarg, "west") == 0)
	    {
	      commandline->position = WEST;
	    }
          else if (g_strcmp0(optarg, "north") == 0)
	    {
	      commandline->position = NORTH;
	    }
          else if (g_strcmp0(optarg, "south") == 0)
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


/* Allocate and initialize the bar data structure */
BarData* init_bar_data()
{
  BarData *bar_data = (BarData *) g_malloc(sizeof(BarData));
  bar_data->color = g_strdup_printf("%s", "FF0000FF");
  bar_data->annotation_is_visible = TRUE;
  bar_data->pencil = TRUE;
  bar_data->grab = TRUE;
  bar_data->text = FALSE;
  bar_data->thickness = 14;
  bar_data->highlighter = FALSE;
  bar_data->rectifier = FALSE;
  bar_data->rounder = FALSE;
  bar_data->arrow = FALSE;

  /* The workspace dir is set to the desktop dir */
  bar_data->workspace_dir = g_strdup_printf("%s", get_desktop_dir());

  return bar_data;
}


/* Create the ardesia bar window */
GtkWidget*
create_bar_window (CommandLine *commandline, GtkWidget *parent)
{
  GtkWidget *bar_window = NULL;
  GError* error = NULL;

  gtkBuilder = gtk_builder_new();
  gchar* file = UI_FILE;
  if (commandline->position>2)
    {
      /* north or south */
      file = UI_HOR_FILE;
    }
  else
    {
      /* east or west */
      if (gdk_screen_height() < 768)
        {
          /* 
           * the bar is too long and then I use an horizontal layout; 
           * this is done to have the full bar for netbook and screen
           * with the 1024x600 resolution 
           */
          file = UI_HOR_FILE;
          commandline->position=NORTH;
        }
    }


  /* load the gtkbuilder file with the definition of the ardesia bar gui */
  gtk_builder_add_from_file(gtkBuilder, file, &error);
  if (error)
    {
      g_warning ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
      g_object_unref (gtkBuilder);
      gtkBuilder = NULL;
      return bar_window;
    }  
  
  BarData *bar_data = init_bar_data();

  /* connect all the callback from gtkbuilder xml file */
  gtk_builder_connect_signals(gtkBuilder, (gpointer) bar_data);
  bar_window = GTK_WIDGET (gtk_builder_get_object(gtkBuilder, "winMain"));
  gtk_window_set_transient_for(GTK_WINDOW(bar_window), GTK_WINDOW(parent));
  if (commandline->decorated)
    {
      gtk_window_set_decorated(GTK_WINDOW(bar_window), TRUE);
    }

  return bar_window;
}


void enable_localization_support()
{
#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain(GETTEXT_PACKAGE);
#endif

  gtk_set_locale();
}


#ifdef linux

void print_trace_line(char *address) {
  char ename[1024];
  int l = readlink("/proc/self/exe",ename,sizeof(ename));
  
  if (l == -1) 
    {
      fprintf(stderr, "failed to find executable\n");
      return;
    }
 	
  ename[l] = 0;

  bfd_init();

  abfd = bfd_openr(ename, 0);
   
  if (!abfd)
    {
      fprintf(stderr, "bfd_openr failed: ");
      return;
    }
   
  /* oddly, this is required for it to work... */
  bfd_check_format(abfd,bfd_object);

  unsigned storage_needed = bfd_get_symtab_upper_bound(abfd);
  syms = (asymbol **) malloc(storage_needed);

  text = bfd_get_section_by_name(abfd, ".text");

  long offset = ((long)address) - text->vma;
   
  if (offset > 0) 
    {
      const char *file;
      const char *func;
      unsigned line;
      if (bfd_find_nearest_line(abfd, text, syms, offset, &file, &func, &line) && file)
	{
	  fprintf(stderr, "file: %s, line: %u, func %s\n",file,line,func);
	}
    }
}


static void print_trace() 
{

  void *array[MAX_FRAMES];
  size_t size;
  size_t i;
  void *approx_text_end = (void*) ((128+100) * 2<<20);

  /* 
   * the glibc functions backtrace is missing on all non-glibc platforms
   */
  
  size = backtrace (array, MAX_FRAMES);
  printf ("Obtained %zd stack frames.\n", size);
  for (i = 0; i < size; i++)
    {
      if (array[i] < approx_text_end)
	{
	  print_trace_line(array[i]);
	}
    }

}
#else
static void print_trace() 
{
  /* 
   * is not yet implemented
   * @TODO does exist a cross plattform way to print the backtrace
   * or we must wait for a cross plattform implementation in non-glibc plattforms?
   */
}


#endif


/* Is called when occurs a sigsegv */
int sigsegv_handler(void *addr, int bad)
{
  print_trace(); 
  exit(2);
}


/* This is the starting point */
int
main(gint argc, char *argv[])
{

  /* Install the SIGSEGV handler */
  if (sigsegv_install_handler(sigsegv_handler)<0)
    {
      exit(2);
    }

  /* Enable the localization support with gettext */
  enable_localization_support();
  
  gtk_init(&argc, &argv);

  set_the_best_colormap();

  CommandLine *commandline = parse_options(argc, argv);

  GtkWidget* background_window = create_background_window(commandline->backgroundimage); 
  
  if (background_window == NULL)
    {
      destroy_background_window();
      g_free(commandline);
      exit(0);
    }

  gtk_window_set_keep_above(GTK_WINDOW(background_window), TRUE); 
  gtk_widget_show(background_window);
  
  set_background_window(background_window);

  
  /** Init annotate */
  annotate_init(background_window, commandline->debug); 

  GtkWidget* annotation_window = get_annotation_window();  

  if (annotation_window == NULL)
    {
      annotate_quit();
      destroy_background_window();
      g_free(commandline);
      exit(0);
    }

  /* annotation window is valid */

  gtk_window_set_keep_above(GTK_WINDOW(annotation_window), TRUE);
  gtk_widget_show(annotation_window);
  
  GtkWidget *ardesia_bar_window = create_bar_window(commandline, annotation_window);
  
  if (ardesia_bar_window == NULL)
    {
      annotate_quit();
      destroy_background_window();
      g_free(commandline);
      exit(0);
    }

  gint width, height;
  gtk_window_get_size(GTK_WINDOW(ardesia_bar_window) , &width, &height);

  /* x and y are the ardesia bar left corner coords */
  gint x, y;
  calculate_initial_position(ardesia_bar_window, 
                             &x, &y, 
                             width, height,
                             commandline->position);
  g_free(commandline);

  /* move the window in the desired position */
  gtk_window_move(GTK_WINDOW(ardesia_bar_window), x, y);  

  gtk_window_set_keep_above(GTK_WINDOW(ardesia_bar_window), TRUE);
  gtk_widget_show(ardesia_bar_window);
  
  gtk_main();
  return 0;
}


