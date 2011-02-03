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


/* Add the xml header */
static void add_header()
{
  gchar* becta_ns = "http://www.becta.org.uk/iwb";
  gchar* svg_ns = "http://www.w3.org/2000/svg";
  gchar* xlink_ns = "http://www.w3.org/1999/xlink"; 
  gchar* iwb_version = "1.0";
  fprintf(fp,"<iwb xmlns:iwb=\"%s\" xmlns:svg=\"%s\" xmlns:xlink=\"%s\" version=\"%s\">\n", becta_ns, svg_ns, xlink_ns, iwb_version);  
}


/* Close the iwb xml tag */
void close_iwb()
{
  fprintf(fp, "</iwb>\n");
}


/* Open the svg tag */
static void open_svg()
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  fprintf(fp,"\t<svg:svg width=\"%d\" height=\"%d\" viewbox=\"0 0 %d %d\">\n", width, height, width, height);
}


/* Close the svg tag */
static void close_svg()
{
  fprintf(fp,"\t</svg:svg>\n");
}


/* Add the background element */
static void add_background()
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  gchar* rgb = "rgb(0,0,0)";
  gchar* opacity = "0";
  
  open_svg();
  fprintf(fp,"\t\t<svg:rect id=\"id1\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" fill-opacity=\"%s\"/>\n", width, height, rgb, opacity);
  close_svg();
}


/* Add the background reference */
static void add_background_reference()
{
  fprintf(fp,"\t<iwb:element ref=\"id1\" background=\"true\"/>\n");
}


/* Add the savepoint element */
static void add_savepoint(gint index)
{
  gint width = gdk_screen_width();
  gint height = gdk_screen_height();
  gchar* id = g_strdup_printf("id%d", index +1);
  gchar* file = g_strdup_printf("images/%s_%d_vellum.png", PACKAGE_NAME, index);
  
  open_svg();

  fprintf(fp,"\t\t<svg:image id=\"%s\" xlink:href=\"%s\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"/>\n", id, file, width, height);
  g_free(file);
  g_free(id);
  close_svg();
}


/* Add the savepoint elements */
static void add_savepoints(gint savepoint_number)
{
  /* for each i call add_savepoint */
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint(i);
    }
}


/* Add the savepoint reference */
static void add_savepoint_reference(gint index)
{
  gchar* id = g_strdup_printf("id%d", index +1);
  fprintf(fp,"\t<iwb:element ref=\"%s\" locked=\"true\"/>\n", id);   
  g_free(id);
}


/* Add the savepoint references */
static void add_savepoint_references(gint savepoint_number)
{
  /* for each i call add_savepoint_reference */
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint_reference(i);
    }
}


/* Create the iwb xml content file */
static void create_xml_content(gchar* content_filename, gchar* img_dir_path)
{
  int savepoint_number = 0;
  GDir *img_dir;  

  fp = fopen(content_filename, "w");

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


/* Create iwb file */
static void create_iwb(gchar* zip_filename, gchar* working_dir, gchar* images_folder, gchar* content_filename)
{
 /* create zip, add all the file inside images to the zip and the content.xml */
  gchar* argv[6] = {"zip", "-r", zip_filename, images_folder, content_filename, (gchar*) 0};

  g_spawn_sync (working_dir /*working_directory*/,
		argv,
		NULL /*envp*/,
		G_SPAWN_SEARCH_PATH,
		NULL /*child_setup*/,
		NULL /*user_data*/,
		NULL /*child_pid*/,
		NULL,
		NULL,
		NULL);
}


/* Export in iwb format */
void export_iwb(gchar* iwb_location)
{
  gchar* iwb_file = NULL;

  /* If the iwb location is null means that it is a new project */
  if (iwb_location == NULL)
    {
      /* will be putted in the project dir */
      gchar* extension = "iwb";
      /* the zip is the iwb in the project inside the ardesia workspace */
      iwb_file = g_strdup_printf("%s%s%s.%s", get_project_dir(), G_DIR_SEPARATOR_S, get_project_name(), extension);
    }
  else
    {
      g_remove(iwb_location);
      iwb_file = g_strdup_printf("%s", iwb_location);
    }

  const gchar* tmpdir = g_get_tmp_dir();
  gchar* content_filename = "content.xml";
  gchar* ardesia_tmp_dir = g_build_filename(tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar* project_name = get_project_name();

  gchar* project_tmp_dir = g_build_filename(ardesia_tmp_dir, project_name, (gchar *) 0);
  gchar* content_filepath = g_build_filename(project_tmp_dir, content_filename, (gchar *) 0); 

  gchar* images = "images";
  gchar* img_dir_path = g_build_filename(project_tmp_dir, images, (gchar *) 0);

  g_remove(content_filepath);
  create_xml_content(content_filepath, img_dir_path);

  create_iwb(iwb_file, project_tmp_dir, "images", content_filename);

  /* add to the list of the artifacts created in the session */
  add_artifact(iwb_file);
  
  g_free(iwb_file);
  g_free(ardesia_tmp_dir);
  g_free(project_tmp_dir); 
  g_free(content_filepath);
  g_free(img_dir_path);
}

