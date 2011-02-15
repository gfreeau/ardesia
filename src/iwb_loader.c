/* 
 * Ardesia-- a program for painting on the screen 
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
#include <iwb_loader.h>
#include <annotation_window.h>


/* Follow the ref and load the associated save-point. */
GSList *
load_savepoint_by_reference (GSList *savepoint_list,
			     gchar *project_tmp_dir,
			     xmlXPathContextPtr context,
			     xmlChar *ref)
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


/* Decompress the iwb file; it is a zip file. */
static void
decompress_iwb (gchar *iwbfile,
		gchar *project_tmp_dir)
{
  /* Unzip the iwb file spawing the Zip-Info process. */
  gchar *argv[5] = {"unzip", iwbfile, "-d", project_tmp_dir, (gchar*) 0};

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
}


/* Add iwb name spaces to the xmlXPathContext. */
static xmlXPathContextPtr register_namespaces (xmlXPathContextPtr context)
{
  xmlXPathRegisterNs (context, (xmlChar *) "iwb", (xmlChar *) "http://www.becta.org.uk/iwb");
  xmlXPathRegisterNs (context, (xmlChar *) "xlink", (xmlChar *) "http://www.w3.org/1999/xlink");
  xmlXPathRegisterNs (context, (xmlChar *) "svg", (xmlChar *) "http://www.w3.org/2000/svg");
  return context;
}


/* Load save-points from iwb */
static GSList *
load_savepoints_by_iwb (GSList *savepoint_list,
			gchar *project_tmp_dir,
			xmlXPathContextPtr context)
{
  xmlChar *xpath_get_element = (xmlChar *) "/iwb/iwb:element";
  xmlXPathObjectPtr result = xmlXPathEvalExpression (xpath_get_element, context);
  gint i =0;

  if (xmlXPathNodeSetIsEmpty (result->nodesetval)){
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
	      xmlFree (background);
	      continue;
	    }
	}

      /* Follow the ref and take xlink href filename. */
      savepoint_list = load_savepoint_by_reference (savepoint_list, project_tmp_dir, context, ref);
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


