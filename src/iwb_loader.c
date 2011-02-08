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


/* Follow the ref and load the associated savepoint */
GSList* load_savepoint_by_reference(GSList* savelist, gchar* project_tmp_dir, xmlXPathContextPtr context, xmlChar* ref)
{
  xmlChar* xpath = (xmlChar *) g_strdup_printf("/iwb/svg:svg/svg:image[@id='%s']", (gchar *) ref);
   
  xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context); 
  g_free((gchar*) xpath);
 
  AnnotateSavepoint *savepoint = g_malloc((gsize) sizeof(AnnotateSavepoint));
   
  xmlNodePtr node = result->nodesetval->nodeTab[0];
  xmlChar* href = xmlGetProp(node, (xmlChar*) "href");
  
  savepoint->filename  = g_build_filename(project_tmp_dir, href, (gchar *) 0);
  
  xmlFree(href);

  /* add to the savepoint list */
  savelist = g_slist_prepend (savelist, savepoint);

  /* Cleanup of XPath data */
  xmlXPathFreeObject(result);

  return savelist;
}


/* Decompress iwb */
static void decompress_iwb(gchar* iwbfile, gchar* project_tmp_dir)
{
  /* Unzip the iwb file */
  gchar* argv[5] = {"unzip", iwbfile, "-d", project_tmp_dir, (gchar*) 0};

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


/* Add iwb namespaces to the xmlXPathContext */
static xmlXPathContextPtr register_namespaces(xmlXPathContextPtr context)
{
  xmlXPathRegisterNs(context, (xmlChar*) "iwb", (xmlChar*) "http://www.becta.org.uk/iwb");
  xmlXPathRegisterNs(context, (xmlChar*) "xlink", (xmlChar*) "http://www.w3.org/1999/xlink");
  xmlXPathRegisterNs(context, (xmlChar*) "svg", (xmlChar*) "http://www.w3.org/2000/svg");
  return context;
}


/* Load savepoints from iwb */
static GSList* load_savepoints_by_iwb(GSList* savelist, gchar* project_tmp_dir, xmlXPathContextPtr context)
{
  xmlChar* xpath_get_element = (xmlChar *) "/iwb/iwb:element";
  xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath_get_element, context);

  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    return savelist;
  }

  /* surf for all the iwb element */
  gint i =0;
  for (i=0; i < result->nodesetval->nodeNr; i++) {
    xmlNodePtr node = result->nodesetval->nodeTab[i];
    xmlChar* ref = xmlGetProp(node, (xmlChar*) "ref");
    xmlChar* background = xmlGetProp(node, (xmlChar*) "background");
    if (background!=NULL)
      {
        if (g_strcmp0((gchar* ) background,"true")==0)
          { 
            xmlFree(background);
	    continue;
          }
      }

    /* follow the ref and take xlink href filename */
    savelist = load_savepoint_by_reference(savelist, project_tmp_dir, context, ref);
    xmlFree(ref);
  }
  /* Cleanup of XPath data */
  xmlXPathFreeObject(result);
  return savelist;
}


/* Load an iwb file and create the list of savepoint */
GSList* load_iwb(gchar* iwbfile)
{
  GSList* savelist = NULL;
  const gchar* tmpdir = g_get_tmp_dir();
  gchar* ardesia_tmp_dir = g_build_filename(tmpdir, PACKAGE_NAME, (gchar *) 0);
  gchar* project_name = get_project_name();

  gchar* project_tmp_dir = g_build_filename(ardesia_tmp_dir, project_name, (gchar *) 0);

  gchar* content_filename = "content.xml";
  gchar* content_filepath = g_build_filename(project_tmp_dir, content_filename, (gchar *) 0);

  decompress_iwb(iwbfile, project_tmp_dir);
  
  /* Init libxml */     
  xmlInitParser();

  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

  xmlDocPtr doc; /* the resulting document tree */

  /*
   * build an XML tree from a the file;
   */
  doc = xmlParseFile (content_filepath);
  if (doc == NULL) {
    g_error("Failed to parse %s\n", content_filepath);
  } 

  xmlXPathContextPtr context = xmlXPathNewContext(doc);

  if(context == NULL) {
    g_error("Error: unable to create new XPath context\n");
    xmlFreeDoc(doc); 
    exit(EXIT_FAILURE);
  }

  context = register_namespaces(context);

  savelist = load_savepoints_by_iwb(savelist, project_tmp_dir, context);

  g_remove(content_filepath);

  xmlXPathFreeContext(context); 
 
  xmlFreeDoc(doc);

  /*
   * Cleanup function for the XML library.
   */
  xmlCleanupParser();

  g_free(ardesia_tmp_dir);
  g_free(project_tmp_dir); 
  g_free(content_filepath);

  return savelist;   
}
