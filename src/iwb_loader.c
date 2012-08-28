/* 
 * Ardesia-- a program for painting on the screen 
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
#include <iwb_loader.h>
#include <annotation_window.h>
#include <background_window.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>


/* Add the background image reference. */
static void
add_background_image_reference (gchar   *project_tmp_dir,
                                xmlChar *href)
{
  /* It is an image */
  gchar *background_path = "";
  background_path = g_build_filename (project_tmp_dir, href, (gchar *) 0);
  set_background_type (2);
  update_background_image (background_path);
}


/* Add the background color reference. */
static void
add_background_color_reference (xmlXPathContextPtr  context,
                                xmlChar            *ref)
{
  xmlChar *xpath = (xmlChar *) g_strdup_printf ("/iwb/svg:svg/svg:rect[@id='%s']", (gchar *) ref);
  xmlXPathObjectPtr result = xmlXPathEvalExpression (xpath, context);
  xmlNodePtr node = result->nodesetval->nodeTab[0];

  /* e.g. rbg(255,255,255) */
  xmlChar *fill = xmlGetProp (node, (xmlChar *) "fill");
  gint open_bracket_index = g_substrlastpos ((const gchar *)fill, "(");
  gint comma_index = g_substrlastpos ((const gchar *)fill, ",");
  gint close_bracket_index = g_substrlastpos ((const gchar *)fill, ")");

  gchar *blue = g_substr ((const gchar *)fill, comma_index+1, close_bracket_index-1);

  guint16 bd =  (guint16) g_ascii_strtoull (blue, NULL, 10);
  g_free(blue);

  gchar *remain_string = g_substr ((const gchar *)fill, open_bracket_index+1, comma_index-1);

  g_free ( (gchar*) fill);

  /* e.g. 255,255 */
  comma_index = g_substrlastpos ((const gchar *)remain_string, ",");

  gchar *red = g_substr ((const gchar *)remain_string, 0, comma_index-1);

  guint16 rd =  (guint16) g_ascii_strtoull (red, NULL, 10);

  g_free(red);

  gchar *green = g_substr ((const gchar *)remain_string, comma_index+1, strlen (remain_string)-1);

  guint16 gd =  (guint16) g_ascii_strtoull (green, NULL, 10);

  g_free(green);

  g_free(remain_string);

  /* e.g. 255 */
  xmlChar *alpha = xmlGetProp (node, (xmlChar *) "fill-opacity");
  guint16 ad =  (guint16) g_ascii_strtoull ((gchar *) alpha, NULL, 10);

  /* FFFFFFFF */

  gchar *rgba = g_strdup_printf ("%02x%02x%02x%02x",
                                 rd,
                                 gd,
                                 bd,
                                 ad);

  if (g_strcmp0 (rgba, "00000000") != 0)
    {
       set_background_type (1);
       update_background_color (rgba);
    }
  else
    { 
       update_background_color (rgba);
       set_background_color ("000000FF");
    }


  g_free( (gchar *) alpha);
  g_free( (gchar *) xpath);
}


/* Follow the ref and load the associated save-point. */
void
load_background_by_reference (gchar               *project_tmp_dir,
                              xmlXPathContextPtr   context,
                              xmlChar             *ref)
{
  xmlChar *xpath = (xmlChar *) g_strdup_printf ("/iwb/svg:svg/svg:image[@id='%s']", (gchar *) ref);
  xmlXPathObjectPtr result = xmlXPathEvalExpression (xpath, context);

  if (result->nodesetval)
    {
      xmlNodePtr node = result->nodesetval->nodeTab[0];
      xmlChar *href = xmlGetProp (node, (xmlChar *) "href");

      if (!href)
        {
          add_background_color_reference (context, ref);
        }
      else
        {
          add_background_image_reference (project_tmp_dir, href);
        }
    }
  /* Cleanup of XPath data. */
  xmlXPathFreeObject (result);
  g_free ((gchar *) xpath);
}


/* Follow the ref and load the associated save-point. */
GSList *
load_savepoint_by_reference (GSList              *savepoint_list,
                             gchar               *project_tmp_dir,
                             xmlXPathContextPtr   context,
                             xmlChar             *ref)
{
  xmlChar *xpath = (xmlChar *) g_strdup_printf ("/iwb/svg:svg/svg:image[@id='%s']", (gchar *) ref);
  xmlXPathObjectPtr result = xmlXPathEvalExpression (xpath, context); 
  AnnotateSavepoint *savepoint = g_malloc ( (gsize) sizeof (AnnotateSavepoint));
  xmlNodePtr node = result->nodesetval->nodeTab[0];
  xmlChar *href = xmlGetProp (node, (xmlChar *) "href");
  g_free ((gchar *) xpath);

  savepoint->filename  = g_build_filename (project_tmp_dir, href, (gchar *) 0);
  
  xmlFree (href);

  /* Add to the save-point list. */
  savepoint_list = g_slist_prepend (savepoint_list, savepoint);

  /* Cleanup of XPath data. */
  xmlXPathFreeObject (result);

  return savepoint_list;
}


/* Decompress infile in dest_dir. */
static void
decompress_infile (GsfInfile *infile,
                   gchar     *dest_dir)
{
  GError   *err = (GError *) NULL;
  int j = 0;

  for (j = 0; j < gsf_infile_num_children (infile); j++)
    {
      GsfInput *child = gsf_infile_child_by_index (infile, j);
      char const* filename = gsf_input_name (child);
      gboolean is_dir = gsf_infile_num_children (GSF_INFILE (child)) >= 0;

      if (is_dir)
        {
          gchar *dir_path = g_build_filename (dest_dir, filename, (gchar *) 0);
          decompress_infile (GSF_INFILE (child), dir_path);
          g_free (dir_path);
        }
      else
        {
          gchar *file_path = g_build_filename (dest_dir, filename, (gchar *) 0);
          GsfOutput *output = GSF_OUTPUT (gsf_output_stdio_new (file_path, &err));
          gsf_input_copy (child, output);
          gsf_output_close (output);
          g_object_unref (output);
          g_free (file_path);
        }

      g_object_unref (G_OBJECT (child));
    }
}


/* Decompress the iwb file; it is a zip file. */
static void
decompress_iwb (gchar *iwbfile,
                gchar *project_tmp_dir)
{
  GError   *err = (GError *) NULL;
  GsfInput   *input = (GsfInput *) NULL;
  GsfInfile  *infile = (GsfInfile *) NULL;
  
  gsf_init ();

  input = gsf_input_stdio_new (iwbfile, &err);

  infile = gsf_infile_zip_new (input, &err);
  g_object_unref (G_OBJECT (input));

  if (infile == NULL)
    {
      g_warning ("Error decompressing iwb file %s: %s", iwbfile, err->message);
      g_error_free (err);
      return;
    }

  decompress_infile (infile, project_tmp_dir);
  g_object_unref (G_OBJECT (infile));
  gsf_shutdown ();
}


/* Add iwb name spaces to the xmlXPathContext. */
static xmlXPathContextPtr register_namespaces (xmlXPathContextPtr context)
{
  xmlXPathRegisterNs (context, (xmlChar *) "iwb", (xmlChar *) "http://www.becta.org.uk/iwb");
  xmlXPathRegisterNs (context, (xmlChar *) "xlink", (xmlChar *) "http://www.w3.org/1999/xlink");
  xmlXPathRegisterNs (context, (xmlChar *) "svg", (xmlChar *) "http://www.w3.org/2000/svg");
  return context;
}


/* Load save-points from iwb. */
static GSList *
load_savepoints_by_iwb (GSList             *savepoint_list,
                        gchar              *project_tmp_dir,
                        xmlXPathContextPtr  context)
{
  xmlChar *xpath_get_element = (xmlChar *) "/iwb/iwb:element";
  xmlXPathObjectPtr result = xmlXPathEvalExpression (xpath_get_element, context);
  gint i =0;

  if (xmlXPathNodeSetIsEmpty (result->nodesetval))
    {
      return savepoint_list;
    }

  /* Surf for all the iwb element. */
  for (i=0; i < result->nodesetval->nodeNr; i++)
    {
      xmlNodePtr node = result->nodesetval->nodeTab[i];
      xmlChar *ref = xmlGetProp (node, (xmlChar *) "ref");
      xmlChar *background = xmlGetProp (node, (xmlChar *) "background");

      if (background)
        {
          if (g_strcmp0 ( (gchar *) background, "true") == 0)
            { 
              load_background_by_reference (project_tmp_dir, context, ref);
              xmlFree (background);
            }
        }
      else
        {
          /* Follow the ref and take xlink href filename. */
          savepoint_list = load_savepoint_by_reference (savepoint_list, project_tmp_dir, context, ref);
        }
      xmlFree (ref);
    }

  /* Cleanup of XPath data. */
  xmlXPathFreeObject (result);
  return savepoint_list;
}


/* Load an iwb file and create the list of save-point. */
GSList *
load_iwb (gchar *iwbfile)
{
  const gchar *tmpdir = g_get_tmp_dir ();
  GSList *savepoint_list = (GSList *) NULL;
  gchar  *ardesia_tmp_dir = g_build_filename (tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar  *project_name = get_project_name ();
  gchar  *project_tmp_dir = g_build_filename (ardesia_tmp_dir, project_name, (gchar *) 0);
  gchar  *content_filename = "content.xml";
  gchar  *content_filepath = g_build_filename (project_tmp_dir, content_filename, (gchar *) 0);
  xmlDocPtr doc = (xmlDocPtr) NULL; // the resulting document tree
  xmlXPathContextPtr context = (xmlXPathContextPtr) NULL;

  decompress_iwb (iwbfile, project_tmp_dir);
  
  /* Initialize libxml. */
  xmlInitParser ();

  /*
   * This initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

    /*
     * Build an XML tree from a the file.
     */
    doc = xmlParseFile (content_filepath);

  if (doc == NULL)
    {
      printf ("Failed to parse %s\n", content_filepath);
      exit (EXIT_FAILURE);
    }

  context = xmlXPathNewContext (doc);

  if (context == NULL)
    {
      xmlFreeDoc (doc); 
      printf ("Error: unable to create new XPath context\n");
      exit (EXIT_FAILURE);
    }

  context = register_namespaces (context);

  savepoint_list = load_savepoints_by_iwb (savepoint_list, project_tmp_dir, context);

  g_remove (content_filepath);

  xmlXPathFreeContext (context);

  xmlFreeDoc (doc);
  doc = NULL;

  /*
   * Cleanup function for the XML library.
   */
  xmlCleanupParser ();

  g_free (ardesia_tmp_dir);
  ardesia_tmp_dir = NULL;

  g_free (project_tmp_dir);
  project_tmp_dir = NULL;

  g_free (content_filepath);
  content_filepath = NULL;

  return savepoint_list;
}


