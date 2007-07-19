#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <time.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define max(A,B) ( (A) > (B) ? (A):(B))
#define SIZE_OF_STRING 80

/*
 *  generates xml from script
 */
xmlDocPtr makeDoc (char *script);

/*
 * build inter-islet network restriction
 */
xmlDocPtr makeXmlInter (gsl_rng * r, char *inter1, char *inter2, char *bpin,
			char *bpout, char *lat, xmlDocPtr doc);

int buildXml (char *script, char *xmldoc);
