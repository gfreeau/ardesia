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
#include <iwb_saver.h>
#include <background_window.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>


/* The file pointer to the iwb file. */
static FILE *fp = NULL;


/* Add the xml header. */
static void
add_header ()
{
  gchar *becta_ns    = "http://www.becta.org.uk/iwb";
  gchar *svg_ns      = "http://www.w3.org/2000/svg";
  gchar *xlink_ns    = "http://www.w3.org/1999/xlink";
  gchar *iwb_version = "1.0";

  fprintf (fp,
           "<iwb xmlns:iwb=\"%s\" xmlns:svg=\"%s\" xmlns:xlink=\"%s\" version=\"%s\">\n",
           becta_ns,
           svg_ns,
           xlink_ns,
           iwb_version);

}


/* Close the iwb xml tag. */
static void
close_iwb ()
{
  fprintf (fp, "</iwb>\n");
}


/* Open the svg tag. */
static void
open_svg ()
{
  gint width  = gdk_screen_width ();
  gint height = gdk_screen_height ();

  fprintf (fp,
           "\t<svg:svg width=\"%d\" height=\"%d\" viewbox=\"0 0 %d %d\">\n",
           width,
           height,
           width,
           height);

}


/* Close the svg tag. */
static void
close_svg ()
{
  fprintf (fp,"\t</svg:svg>\n");
}


/* Add the savepoint element. */
static void
add_savepoint (gint index)
{
  gint width = gdk_screen_width ();
  gint height = gdk_screen_height ();
  gchar *id = g_strdup_printf ("id%d", index +1);
  gchar *file = g_strdup_printf ("images/%s_%d_vellum.png", PACKAGE_NAME, index);

  open_svg ();

  fprintf (fp,
           "\t\t<svg:image id=\"%s\" xlink:href=\"%s\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"/>\n",
           id,
           file,
           width,
           height);

  g_free (file);
  file = NULL;
  g_free (id);
  close_svg ();
}


/* Add the background element. */
static void
add_background (gchar *img_dir_path,
                gchar *background_image)
{
  gint width  = gdk_screen_width ();
  gint height = gdk_screen_height ();
  gchar *image_destination_path = (gchar *) NULL;

  image_destination_path = g_build_filename (img_dir_path, "ardesia_0_vellum.png", (gchar *) 0);

  /* Background image valid and set to image type */
  if ((background_image) && (get_background_type ()==2))
    {
      /* if the background is oupdated */
      if (g_strcmp0 (background_image, image_destination_path) != 0)
        {
          /* copy the file in ardesia_0_vellum.png under image_path overriding it */
          GFile *image_destination = g_file_new_for_path(image_destination_path);
          GFile *image_source = g_file_new_for_path(background_image);

          g_file_copy (image_source,
                       image_destination,
                       G_FILE_COPY_OVERWRITE,
                       NULL,
                       NULL,
                       NULL,
                       NULL);

          g_object_unref (image_source);
          g_object_unref (image_destination);
        }
      add_savepoint (0);
    }
  else
    {
      gchar *color = get_background_color();
      /* Initialize the rgba components to transparent */
      guint r = 0;
      guint g = 0;
      guint b = 0;
      guint a = 0;

      /* If the background type is colour then parse it */
      if ((color!=NULL) && (get_background_type ()!=0))
        {
          sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);
        }

      gchar *rgb  =  g_strdup_printf ("rgb(%d,%d,%d)", r, g, b);

      open_svg ();
      fprintf (fp,
               "\t\t<svg:rect id=\"id1\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" fill-opacity=\"%d\"/>\n",
               width,
               height,
               rgb,
               a);
      close_svg ();
      g_free(rgb);
    }

  g_free(image_destination_path);
}


/* Add the background reference. */
static void
add_background_reference ()
{
  fprintf (fp,
           "\t<iwb:element ref=\"id1\" background=\"true\"/>\n");
}


/* Add the savepoint elements. */
static void
add_savepoints (gint savepoint_number)
{
  /* For each i call add_savepoint. */
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint (i);
    }
}


/* Add the savepoint reference. */
static void
add_savepoint_reference (gint index)
{
  gchar *id = g_strdup_printf ("id%d", index +1);
  fprintf (fp,"\t<iwb:element ref=\"%s\" locked=\"true\"/>\n", id);
  g_free (id);
}


/* Add the savepoint references. */
static void
add_savepoint_references (gint savepoint_number)
{
  /* For each i call add_savepoint_reference. */
  gint i=1;
  for (i=1; i<=savepoint_number; i++)
    {
      add_savepoint_reference (i);
    }
}


/* Create the iwb xml content file. */
static void
create_xml_content (gchar *content_filename,
                    gchar *img_dir_path,
                    gchar *background_image)
{
  int savepoint_number = -1;
  GDir *img_dir;  

  fp = fopen (content_filename, "w");

  add_header ();
  add_background (img_dir_path, background_image);

  if (!background_image)
    {
      savepoint_number = 0;
    }

  img_dir = g_dir_open (img_dir_path, 0, NULL);

  while (g_dir_read_name (img_dir))
    {
      savepoint_number++;
    }

  g_dir_close (img_dir);

  add_savepoints (savepoint_number);
  add_background_reference ();
  add_savepoint_references (savepoint_number);
  close_iwb ();
  fclose (fp);
}


/* Add the filename under path to the gst_outfile. */
static void add_file_to_gst_outfile (GsfOutfile   *out_file,
                                     gchar        *path,
                                     const gchar  *file_name)
{
  GError   *err = (GError *) NULL;
  gchar *file_path = g_build_filename (path, file_name, NULL);
  GsfInput *input = GSF_INPUT (gsf_input_stdio_new (file_path, &err));
  GsfOutput  *child  = gsf_outfile_new_child (out_file, file_name, FALSE);

  gsf_input_copy (input, child);
  gsf_output_close (child);
  g_object_unref (child);

  g_free (file_path);
}


/* Add all the files in the folder under the working_dir to the gst_outfile. */
static void
add_folder_to_gst_outfile  (GsfOutfile  *gst_outfile,
                            gchar       *working_dir,
                            gchar       *folder)
{
  GsfOutfile *gst_dir = GSF_OUTFILE (gsf_outfile_new_child  (gst_outfile, folder, TRUE));
  gchar *path = g_build_filename (working_dir, folder, NULL);
  GDir *dir = g_dir_open (path, 0, NULL);

  if (dir)
    {
      const gchar *file = (const gchar *) NULL;

      while ( (file = g_dir_read_name (dir)))
        {
          add_file_to_gst_outfile (gst_dir, path, file);
        }

      g_dir_close (dir);
    }

  gsf_output_close ((GsfOutput *) gst_dir);
  g_object_unref (gst_dir);
  g_free (path);
}


/* Create the iwb file. */
static void
create_iwb (gchar *zip_filename,
            gchar *working_dir,
            gchar *images_folder,
            gchar *content_filename)
{
  GError   *err = (GError *) NULL;
  GsfOutfile *gst_outfile  = (GsfOutfile *) NULL;
  GsfOutput  *gst_output = (GsfOutput *) NULL;

  gsf_init ();

  gst_output = gsf_output_stdio_new (zip_filename, &err);
  if (gst_output == NULL) 
    {
      g_warning ("Error saving iwb: %s\n", err->message);
      g_error_free (err);
      return;
    }

  gst_outfile  = gsf_outfile_zip_new (gst_output, &err);
  if (gst_outfile == NULL)
    {
      g_warning ("Error in gsf_outfile_zip_new: %s\n", err->message);
      g_error_free (err);
      return;
    }

  g_object_unref (G_OBJECT (gst_output));

  add_folder_to_gst_outfile (gst_outfile, working_dir, images_folder);

  add_file_to_gst_outfile (gst_outfile, working_dir, content_filename);

  gsf_output_close ((GsfOutput *) gst_outfile);
  g_object_unref (G_OBJECT (gst_outfile));

  gsf_shutdown ();  
}


/* Export in the iwb format. */
void
export_iwb (gchar *iwb_location)
{
  gchar *iwb_file = (gchar *) NULL;
  const gchar *tmpdir = g_get_tmp_dir ();
  gchar *content_filename = "content.xml";
  gchar *ardesia_tmp_dir = g_build_filename (tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar *project_name = get_project_name ();

  gchar *project_tmp_dir = g_build_filename (ardesia_tmp_dir, project_name, (gchar *) 0);
  gchar *content_filepath = g_build_filename (project_tmp_dir, content_filename, (gchar *) 0);

  gchar *images = "images";
  gchar *img_dir_path = g_build_filename (project_tmp_dir, images, (gchar *) 0);

  gchar *background_image = get_background_image();

  gchar *first_savepoint_file = g_strdup_printf ("%s%s%s_2_vellum.png", img_dir_path, G_DIR_SEPARATOR_S, PACKAGE_NAME);

  /* if exist the file I continue to save */
  if ((file_exists(first_savepoint_file)) || (background_image))
    {
      
      /* If the iwb location is null means that it is a new project. */
      if (iwb_location == NULL)
        {
          /* It will be putted in the project dir. */
          gchar *extension = "iwb";
          gchar *iwb_name =  g_strdup_printf("%s.%s", get_project_name (), extension);

          /* The zip file is the iwb file located in the ardesia workspace. */
          iwb_file = g_build_filename (get_project_dir (), iwb_name, (gchar *) 0);
          g_free(iwb_name);
        }
      else
        {
          g_remove (iwb_location);
          iwb_file = g_strdup_printf ("%s", iwb_location);
        }

      g_remove (content_filepath);

      create_xml_content (content_filepath, img_dir_path, background_image);

      create_iwb (iwb_file, project_tmp_dir, "images", content_filename);

      /* Add to the list of the artefacts created in the session. */
      add_artifact (iwb_file);
    }

  g_free (first_savepoint_file);
  g_free (iwb_file);
  g_free (ardesia_tmp_dir);
  g_free (project_tmp_dir);
  g_free (content_filepath);
  g_free (img_dir_path);
}


