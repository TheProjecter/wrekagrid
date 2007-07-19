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

#include <dirent.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlsave.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wrekavoc.h"
#include "builder.h"
#include "utils.h"
#include "timings.h"

//!  Buffer size for TCP
#define TCP_BUFFER_SIZE (32*1024)
#define TCP_TIMING_SIZE (256*1024)
/*TCP_PIECE_SIZE must divide TCP_TIMING_SIZE*/
#define TCP_PIECE_SIZE (TCP_TIMING_SIZE/32)

/**
 * \brief function to check if xml structure is correct
 *
 * \param xmldoc 
 * \return exit code, 0 if ok
 * \see ---
 *
 */
int checkXml (char *xmldoc)
{
  xmlNodePtr cur;
  xmlDocPtr doc = xmlParseFile (xmldoc);

  if (doc == NULL) {
    perror ("Document not parsed successfully.");
    exit (-1);
  }

  cur = xmlDocGetRootElement (doc);

  if (cur == NULL) {
    xmlFreeDoc (doc);
    perror ("Empty document");
    exit (-2);
  }

  if (xmlStrcmp (cur->name, (const xmlChar *) "configuration")) {
    xmlFreeDoc (doc);
    perror ("Document of the wrong type, root node != configuration");
    exit (-3);
  }

  xmlFreeDoc (doc);
  return 0;
}

/**
 * \brief function to send xml structure to nodes
 *
 * \param xmldoc xml structure to send
 * \return exit code, 0 if ok
 * \see ---
 *
 */
int sendXml (char *xmldoc)
{
  FILE *fd;
  xmlDocPtr doc;
  xmlNodePtr node1, node2;
  xmlChar *islet_name, *node_name;
  int nb;
  char buff[512];
  int sockfd[100000], nbsock, i = 0;

  /* Each server ip adress are read in the xml file and a socket
   * is created for each and stored in an array */
  doc = xmlParseFile (xmldoc);
  node1 = doc->children->children->next;
  while (node1 && !strcmp (node1->name, "islet")) {
    islet_name = xmlGetProp (node1, "name");
    node2 = node1->children->next->next->next;
    while (node2) {
      node_name = xmlGetProp (node2, "name");
      printf ("  -> sending configuration to %s in %s\n",
	      node_name, islet_name);
      sockfd[i] = startClientSocket (PORT, node_name);
      if ((write (sockfd[i], "order\0    ", 10)) < 0) {
	perror ("ERROR writing to socket");
	exit (-8);
      }

      xmlFree (node_name);
      node2 = node2->next->next;
      i++;
    }
    xmlFree (islet_name);
    node1 = node1->next->next;
  }
  xmlFreeDoc (doc);
  nbsock = i;

  if ((fd = fopen (xmldoc, "r")) == NULL) {
    perror ("Error opening the configuration file");
    exit (-7);
  }

  /* Each string of 512 char from the configuration file are sent to
   * each socket. In this way, the file is read only one time, but
   * there is probably a better solution for sending the same data to
   * several socket or receiver */
  //TODO:Improve the transfert of data, if possible
  while (!feof (fd)) {
    nb = fread (buff, 1, 512, fd);
    for (i = 0; i < nbsock; i++) {
      write (sockfd[i], buff, nb);
    }
  }

  for (i = 0; i < nbsock; i++) {
    shutdown (sockfd[i], 1);
  }
  fclose (fd);
  return 0;
}

/**
 * \brief function to kill wrekavocd on nodes
 *
 * \param xmldoc 
 * \return exit code, 0 if ok
 * \see ---
 *
 */
int killHost (char *xmldoc)
{
  int fd;
  xmlDocPtr doc;
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  int size;
  xmlNodeSetPtr nodes;
  int i;

  printf ("Loading Xml...\n\n");
  if ((fd = open (xmldoc, O_RDONLY)) == -1) {
    perror ("Error opening the configuration file");
    exit (-7);
  }

  printf ("Checking Xml...\n\n");
  checkXml (xmldoc);
  doc = xmlParseFile (xmldoc);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ("//machine", xpathCtx);

  nodes = xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;

  for (i = size - 1; i >= 0; i--) {
    int sockfd =
      startClientSocket (PORT, xmlGetProp (nodes->nodeTab[i], "name"));
    printf ("  -> kill %s\'s server...\n",
	    xmlGetProp (nodes->nodeTab[i], "name"));
    if ((write (sockfd, "quit\0     ", 10)) < 0) {
      perror ("ERROR writing to socket");
      exit (-8);
    }
    close (sockfd);
  }

  xmlFreeDoc (doc);
  close (fd);
  return 0;
}

/**
 * \brief function to resetconfiguration
 *
 * \param xmldoc 
 * \return exit code, 0 if ok
 * \see ---
 *
 */
int Back2normalHost (char *xmldoc)
{
  int fd;
  xmlDocPtr doc;
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  int size;
  xmlNodeSetPtr nodes;
  int i;

  printf ("Loading Xml...\n\n");
  if ((fd = open (xmldoc, O_RDONLY)) == -1) {
    perror ("Error opening the configuration file");
    exit (-7);
  }

  printf ("Checking Xml...\n\n");
  checkXml (xmldoc);
  doc = xmlParseFile (xmldoc);

  printf ("Sending Xml...\n");

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ("//machine", xpathCtx);

  nodes = xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;

  for (i = size - 1; i >= 0; i--) {
    int sockfd =
      startClientSocket (PORT, xmlGetProp (nodes->nodeTab[i], "name"));
    printf ("  -> reseting %s\'s state...\n",
	    xmlGetProp (nodes->nodeTab[i], "name"));
    if ((write (sockfd, "normal\0   ", 10)) < 0) {
      perror ("ERROR writing to socket");
      exit (-8);
    }
    close (sockfd);
  }

  xmlFreeDoc (doc);
  close (fd);
  return 0;
}

/**
 * \brief function to check a node
 *
 * \param name name of the node to check
 * \param target ---
 * \return exit code, 0 if ok
 * \see ---
 *
 */
int checkHost (char *name, char *target)
{
  char buff;
  /*	Open a socket	*/
  int sockfd = startClientSocket (PORT, name);
  /*	Send "check"	*/
  if ((write (sockfd, "check\0    ", 10)) < 0) {
    perror ("ERROR writing to socket");
    exit (-8);
  }

  printf ("Checking %s state...\n\n", name);

  if ((write (sockfd, target, 1 + strlen (target))) < 0) {
    perror ("ERROR writing to socket");
    exit (-8);
  }

  printf ("Current configuration on %s :\n", name);

  /* The client will receive data until the server decide to
   * shut down the connection */
  while (read (sockfd, &buff, 1) != 0)
    printf ("%c", buff);

  close (sockfd);
  return 0;
}

/**
 * \brief Main function of wrekavoc client
 *
 * \param argc number ao argument passed to binaries file
 * \param argv pointer to Array pointer that contains arguments
 * \return exit code
 * \see ---
 *
 */
int main (int argc, char **argv)
{
  char xmldoc[SIZE_OF_STRING] = "\0";
  strcat (xmldoc, getenv ("HOME"));
  strcat (xmldoc, CLIENT_CONF_DIR);
  if (opendir (xmldoc) == NULL)
    mkdir (xmldoc, 0755);
  strcat (xmldoc, CLIENT_CONF_FILE);

  /*  Is there enough parameters	*/
  if (argc <= 1) {
    printf ("Usage: %s <script>|<-c host [rhost] >|<-r [script] >|<-q>\n",
	    argv[0]);
    return (0);
  }

  /*  Option -r reset all wrekavocd	*/
  if (argc <= 3 && argv[1][0] == '-' && argv[1][1] == 'r') {
    if (argc <= 2)
      Back2normalHost (xmldoc);
    else {
      buildXml (argv[2], "/tmp/wrekavoc.reset");
      Back2normalHost ("/tmp/wrekavoc.reset");
    }
  } 
  /*  Option -c : check host	*/
  else if (argc <= 4 && argv[1][0] == '-' && argv[1][1] == 'c') {
    if (argc == 3)
      checkHost (argv[2], "a");
    else if (argc == 4)
      checkHost (argv[2], argv[3]);
  } 
  /*  Option -q : quit all wrekavocd	*/
  else if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'q') {
    killHost (xmldoc);
  } 
  /*  Normal : transfert data to nodes	*/
  else {
    /*  Create xml structure from script	*/
    printf ("Building Xml...\n\n");
    buildXml (argv[1], xmldoc);
    /*  Check if xml structure is ok	*/
    printf ("Checking Xml...\n\n");
    checkXml (xmldoc);
    /*  Send xml structure to nodes	*/
    printf ("Sending Xml...\n");
    sendXml (xmldoc);
  }
  printf ("\nAction finished!\n");
  return 0;
}
