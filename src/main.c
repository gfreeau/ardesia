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
 * Initial main.c.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "stdlib.h"
#include "unistd.h"

#include <X11/Xutil.h>

int annotatepid;

/* Get the screen resolution asking to the Xorg server throught the xlib libraries */
int 
getScreenResolution(Display *display, int *width, int *height)
{
  Screen *screen = XDefaultScreenOfDisplay(display);
  *width  = XWidthOfScreen (screen);
  *height = XHeightOfScreen(screen);
  return 0;
}


/* 
 * Calculate the position where move the main window 
 * the bottom centered position upon the window manager bar
 */
void 
calculateBottomCenteredPosition(GtkWidget *mainWindow, int dwidth, int dheight, int *x, int *y, int *wwidth, int *wheight)
{
  GtkRequisition requisition;
  gtk_widget_size_request(mainWindow, &requisition);
  *wwidth  = requisition.width;
  *wheight = requisition.height;
  int spaceFromBottom = 30;
  *x = (dwidth - *wwidth)/2;
  *y = dheight - spaceFromBottom - *wheight;  
}


/* 
 * Move the main windowin the bottom centered position
 * Here can be beatiful have a configuration file
 * where put the user can decide the position of the window
 */
int 
move(GtkWidget *mainWindow, int *x, int *y, int *wwidth, int *wheight)

{
  Display *display = XOpenDisplay (0);  
  int dwidth, dheight;
  if (getScreenResolution(display, &dwidth, &dheight)<0) 
    {
      printf ("Fatal error while getting screen resolution\n");
      return -1;
    }
  calculateBottomCenteredPosition(mainWindow,dwidth,dheight, x, y, wwidth, wheight);
  gtk_window_move(GTK_WINDOW(mainWindow),*x,*y);  
  return 0; 
}


/* 
 * Start the annotate process that
 * is the core part of ardesia; 
 * this will satisfy the desire of the user
 * listening the rpc messages 
 * sended by the ardesia interface
 */
int
startAnnotate(int x, int y, int width, int height)

{
  pid_t pid;

  pid = fork();

  if (pid < 0)
    {
      return -1;
    }
  if (pid == 0)
    {
      char* annotate="annotate";
      char* annotatebin = (char*) malloc(160*sizeof(char));
      sprintf(annotatebin,"%s/../bin/%s", PACKAGE_DATA_DIR, annotate);
      char* xa = (char*) malloc(16*sizeof(char));
      sprintf(xa,"%d",x);
      char* ya = (char*) malloc(16*sizeof(char));
      sprintf(ya,"%d",y);
      char* widtha = (char*) malloc(16*sizeof(char));
      sprintf(widtha,"%d",width);
      char* heighta = (char*) malloc(16*sizeof(char));
      sprintf(heighta,"%d",height);
      
      execl(annotatebin,annotate,"--rectangle", xa, ya, widtha, heighta, NULL);

      /*
	execl("/usr/bin/valgrind", "valgrind" , "--leak-check=full", "--show-reachable=yes", "--log-file=valgrind.log",
	annotatebin,"--rectangle", xa, ya, widtha, heighta, NULL);
      */
      
      free(annotatebin);
      free(xa);
      free(ya);
      free(widtha);
      free(heighta);
    }
  return pid;
}


/* 
 * The main entry of the program;
 * not all the koders 
 * start to read the code by here 
 * a lot of 
 * are only patchers 
 * bad merchants
 * hopeless 
 * and without any horizon
 * avoid to 
 * stay closed to 
 * this type of people
 * if you are not immune from 
 * this type of
 * enemy that 
 * tains the world 
 * the slavery
 */
int
main (int argc, char *argv[])

{
  GtkWidget *mainWindow;

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  int x, y, width, height;

  /*
   * The following code create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  mainWindow = create_mainWindow ();
 
  /* move the window iin the desired position */
  move(mainWindow,&x,&y,&width,&height);
   
  /** Launch annotate */
  annotatepid = startAnnotate(x,y,width,height);
  
  gtk_widget_show (mainWindow);
  
  gtk_main ();
  gtk_widget_destroy(mainWindow); 

  return 0;
}
