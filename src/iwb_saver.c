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

#include <utils.h>
#include <iwb_saver.h>

static FILE* fp = NULL;


static void add_header()
{
  gchar* becta_ns = "http://www.becta.org.uk/iwb";
  gchar* svg_ns = "http://www.w3.org/2000/svg";
  gchar* xlink_ns = "http://www.w3.org/1999/xlink"; 
  gchar* iwb_version = "1.0";
  fprintf(fp,"<iwb xmlns:iwb=\"%s\" xmlns:svg=\"%s\" xmlns:xlink=\"%s\" version=\"%s\">\n", becta_ns, svg_ns, xlink_ns, iwb_version);  
}


void close_iwb()
{
  fprintf(fp, "</iwb>\n");
}


static void open_svg()
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  fprintf(fp,"\t<svg:svg width=\"%d\" height=\"%d\" viewbox=\"0 0 %d %d\">\n", width, height, width, height);
}

static void close_svg()
{
  fprintf(fp,"\t</svg:svg>\n");
}


static void add_background()
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  open_svg();
  gchar* rgb = "rgb(0,0,0)";
  gchar* opacity = "0";
  fprintf(fp,"\t\t<svg:rect id=\"id1\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" fill-opacity=\"%s\"/>\n", width, height, rgb, opacity);
  close_svg();
}


static void add_background_reference()
{
  fprintf(fp,"\t<iwb:element ref=\"id1\" background=\"true\"/>\n");
}


static void add_savepoint(gint index)
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  open_svg();
  gchar* id = g_strdup_printf("id%d", index +1);
  gchar* file = g_strdup_printf("images%s%s_%d_vellum.png", G_DIR_SEPARATOR_S, PACKAGE_NAME, index);

  fprintf(fp,"\t\t<svg:image id=\"%s\" xlink:href=\"%s\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"/>\n", id, file, width, height);
  g_free(file);
  g_free(id);
  close_svg();
}


static void add_savepoints(gint savepoint_number)
{
  //for each i call add_savepoint
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint(i);
    }
}


static void add_savepoint_reference(gint index)
{
  gchar* id = g_strdup_printf("id%d", index +1);
  fprintf(fp,"\t<iwb:element ref=\"%s\" locked=\"true\"/>\n", id);   
  g_free(id);
}


static void add_savepoint_references(gint savepoint_number)
{
  //for each i call add_savepoint_reference
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint_reference(i);
    }
}


static void create_content(gchar* content_filename, gchar* img_dir_path)
{
  fp = fopen(content_filename, "w");

  int savepoint_number = 0;
 
  GDir *img_dir;
  img_dir = g_dir_open(img_dir_path, 0, NULL);

  while ((g_dir_read_name(img_dir)))
    {
      savepoint_number++;
    }
  
  g_dir_close(img_dir);

  add_header();
  add_background();
  add_savepoints(savepoint_number);  
  add_background_reference();
  add_savepoint_references(savepoint_number);
  close_iwb();
  fclose(fp);
}

/* export in iwb format */
void export_iwb(gchar* location)
{
  const gchar* tmpdir = g_get_tmp_dir();
  gchar* content_filename = "content.xml";
  gchar* ardesia_tmp_dir = g_strdup_printf("%s%s%s", tmpdir, G_DIR_SEPARATOR_S, PACKAGE_NAME);
  gchar* project_name = get_project_name();

  gchar* project_tmp_dir = g_strdup_printf("%s%s%s", ardesia_tmp_dir, G_DIR_SEPARATOR_S, project_name);
  gchar* content_filepath = g_strdup_printf("%s%s%s", project_tmp_dir, G_DIR_SEPARATOR_S, content_filename); 

  gchar* images = "images";
  gchar* img_dir_path = g_strdup_printf("%s%s%s", project_tmp_dir, G_DIR_SEPARATOR_S, images);

  create_content(content_filepath, img_dir_path);

  /* will be putted in the project dir */
  gchar* extension = "iwb";
  /* the zip is the iwb in the project inside the ardesia workspace */
  gchar* zip_filename = g_strdup_printf("%s%s%s.%s", location, G_DIR_SEPARATOR_S, project_name, extension);

  /* create zip, add all the file inside images to the zip and the content.xml */
  gchar* argv[6] = {"zip", "-r", zip_filename, "images", content_filename, (gchar*) 0};
  g_spawn_sync (project_tmp_dir /*working_directory*/,
		argv,
		NULL /*envp*/,
		G_SPAWN_SEARCH_PATH,
		NULL /*child_setup*/,
		NULL /*user_data*/,
		NULL /*child_pid*/,
		NULL,
		NULL,
		NULL);
  

  g_free(ardesia_tmp_dir);
  g_free(project_tmp_dir); 
  g_free(content_filepath);
  g_free(img_dir_path);
  
}

