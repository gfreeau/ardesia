/* 
 * Annotate -- a program for painting on the screen 
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

#include "time.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <string.h> 
#include <gconf/gconf-client.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <gdk/gdk.h>


/* Take a GdkColor and return the RGB string */
char* gdkcolor_to_rgb(GdkColor* gdkcolor)
{
  char*   ret= malloc(7*sizeof(char));;
  /* transform in the  RGB format e.g. FF0000 */ 
  sprintf(ret,"%02x%02x%02x", gdkcolor->red/257, gdkcolor->green/257, gdkcolor->blue/257);
  return ret;
}


/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor 
 * neglecting the alpha channel
 */
GdkColor* rgb_to_gdkcolor(char* rgb)
{
   GdkColor* gdkcolor = g_malloc (sizeof (GdkColor));
   gchar    *ccolor = malloc(7);
   ccolor[0]='#';
   strncpy(&ccolor[1], rgb, 6);
   ccolor[7]=0;
   gdk_color_parse (ccolor, gdkcolor);
   g_free(ccolor);
   return gdkcolor;
}


/* Get the current date and format in a printable format */
char* get_date()
{
  struct tm* t;
  time_t now;
  time( &now );
  t = localtime( &now );

  char* date = malloc(64*sizeof(char));
  sprintf(date, "%d-%d-%d_%d:%d:%d", t->tm_mday,t->tm_mon+1,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec);
  return date;
}


/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir)
{
  char* afterslash = strrchr(filename, '/');
  if (afterslash == 0)
    {
      /* relative path */
      filename = strcat(filename,desktop_dir);
    }
  struct stat statbuf;
  if(stat(filename, &statbuf) < 0) {
    if(errno == ENOENT) {
      return FALSE;
    } else {
      perror("");
      exit(0);
    }
  }
  printf("filename %s exists \n", filename);
  return TRUE;
}


/*
 * Get the desktop folder;
 * Now this function use gconf to found the folder,
 * this means that this rutine works properly only
 * with the gnome desktop environment
 * We can investigate how-to do this
 * in a desktop environment independant way
 */
const gchar * get_desktop_dir (void)
{
  GConfClient *gconf_client = NULL;
  gboolean desktop_is_home_dir = FALSE;
  const gchar* desktop_dir = NULL;

  gconf_client = gconf_client_get_default ();
  desktop_is_home_dir = gconf_client_get_bool (gconf_client,
                                               "/apps/nautilus/preferences/desktop_is_home_dir",
                                               NULL);
  if (desktop_is_home_dir)
    {
      desktop_dir = g_get_home_dir ();
    }
  else
    {
      desktop_dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
    }

  g_object_unref (gconf_client);

  return desktop_dir;
}


