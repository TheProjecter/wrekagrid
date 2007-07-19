/**
* Copyright 
* Jens GUSTEDT (INRIA) jens.gustedt@loria.fr
* Emmanuel JEANNOT (INRIA) emmanuel.jeannot@loria.fr
* Marc THIERRY (INRIA, 2004)
* Louis-Claude CANON (INRIA, 2005)
* Olivier DUBUISSON (INRIA 2007) olivier.dubuisson@loria.fr
*
* This software is a computer program whose purpose is to define and 
* control the heterogeneity of a given platform by degrading CPU, network 
* or memory capabilities of each node composing this platform.
* The degradation is done remotely, without restarting the hardware. 
* The control is fine, reproducible and independent 
* (one may degrades CPU without modifying the network bandwidth).
* 
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security. 
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/

#include "builder.h"
#include "utils.h"

/*  generates xml from script */
/**
 * \brief function to make XML structure from script
 *
 * \param script String that contains script
 * \return xmlDocPtr return XML structure
 * \see buildXml
 *
 */
xmlDocPtr makeDoc (char *script)
{
  //the document that will contain the xml
  xmlDocPtr doc;

  //xml nodes needed to move around in the xml document
  xmlNodePtr root_node = NULL,	/* node pointers */
    node = NULL, node1 = NULL, node2 = NULL, node3 = NULL;

  //needed to set the type of random distribution
  struct distrib *distribRange, *distrib;

  //the file where the script will be put
  FILE *in;

  //string where data from script will be stored
  char inter[SIZE_OF_STRING], name[SIZE_OF_STRING],
    range[10000 * SIZE_OF_STRING], seed[SIZE_OF_STRING], cpu[SIZE_OF_STRING],
    bpin[SIZE_OF_STRING], bpout[SIZE_OF_STRING], lat[SIZE_OF_STRING],
    mem[SIZE_OF_STRING], user[SIZE_OF_STRING];

  //useful variables
  char machname1[SIZE_OF_STRING];
  int i;

  /* Creates a new document, a node and set it as a root node */
  doc = xmlNewDoc (BAD_CAST "1.0");
  root_node = xmlNewNode (NULL, BAD_CAST "configuration");
  xmlDocSetRootElement (doc, root_node);

  //open the script file
  if ((in = fopen (script, "r")) == NULL) {
    perror ("Unable to open the file");
    exit (-9);
  }
  //while there is still something to be read in the script
  while (!feof (in)) {
    //we scan the first bit of the script
    fscanf (in, "%s", name);
    //char '!' defines if we are defining islet or not.
    //if there is no '!' we are defining islet
    if (name[0] != '!') {
      //we get the data from de script
      fscanf (in, " : %s {\n", range);
      fscanf (in, "SEED :%s\n", seed);
      fscanf (in, "CPU :%s\n", cpu);
      fscanf (in, "BPOUT :%s\n", bpout);
      fscanf (in, "BPIN :%s\n", bpin);
      fscanf (in, "LAT :%s\n", lat);
      fscanf (in, "USER :%s\n", user);
      fscanf (in, "MEM :%s\n}\n", mem);

      //root element defining islet
      node = xmlNewChild (root_node, NULL, BAD_CAST "islet", NULL);
      xmlNewProp (node, BAD_CAST "name", name);

      node1 = xmlNewChild (node, NULL, BAD_CAST "spec", NULL);
      node2 = xmlNewChild (node1, NULL, BAD_CAST "cpu", NULL);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "frequency", cpu);

      node2 = xmlNewChild (node1, NULL, BAD_CAST "network", NULL);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "in_flow", bpin);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "out_flow", bpout);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "latency", lat);

      node2 = xmlNewChild (node1, NULL, BAD_CAST "resources", NULL);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "user", user);
      node3 = xmlNewChild (node2, NULL, BAD_CAST "maxmalloc", mem);

      node2 = xmlNewChild (node1, NULL, BAD_CAST "seed", seed);

      // Getting the range of the islet
      distribRange = defDistrib (range);

      i = 0;
      while (ithName (distribRange, i++, machname1)) {
	node1 = xmlNewChild (node, NULL, BAD_CAST "machine", NULL);
	xmlNewProp (node1, BAD_CAST "name", machname1);
      }
      free (distribRange);
    } else {
      fscanf (in, " :%s %s %s %s %s\n", inter, bpin, bpout, lat, seed);

      node = xmlNewChild (root_node, NULL, BAD_CAST "network_spec", NULL);

      distrib = defDistrib (inter);
      xmlNewProp (node, BAD_CAST "from", distrib->arg1);
      xmlNewProp (node, BAD_CAST "to", distrib->arg2);
      free (distrib);

      node1 = xmlNewChild (node, NULL, BAD_CAST "AB_flow", bpin);
      node1 = xmlNewChild (node, NULL, BAD_CAST "BA_flow", bpout);
      node1 = xmlNewChild (node, NULL, BAD_CAST "latency", lat);
      node1 = xmlNewChild (node, NULL, BAD_CAST "seed", seed);
    }
  }
  fclose (in);
  return doc;
}

/**
 * \brief function to create file XML from script
 *
 * \param script String that contains script
 * \param xmldoc file to write xml structure
 * \return int allways 0
 * \see ---
 *
 */
int buildXml (char *script, char *xmldoc)
{
  /* Document pointer */
  xmlDocPtr doc = makeDoc (script);

  /* Dumping document to stdio or file */
  xmlSaveFormatFileEnc (xmldoc, doc, "UTF-8", 1);

  /* Free the document */
  xmlFreeDoc (doc);

  /* Free the global variables that may
   * have been allocated by the parser. */
  xmlCleanupParser ();

  /* This is to debug memory for regression tests */
  xmlMemoryDump ();

  return (0);
}
