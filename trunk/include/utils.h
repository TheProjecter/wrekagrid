#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>

#include "builder.h"

#define BACKLOG 10		// how many pending connections queue will hold
#define PORT 3490		// the port client will be connecting to order
#define PORT_LATENCY 3493	// the port client will be connecting to normalize
//#define PORT_ORDER 3490 // the port client will be connecting to order
//#define PORT_CHECK 3491 // the port client will be connecting to check
//#define PORT_NORMAL 3492 // the port client will be connecting to normalize

char *getip ();

ssize_t my_read (int fd, void *buf, size_t count, int line, char *filename);

ssize_t raw_send_in_pieces (int socket, unsigned char *data_buffer,
			    size_t data_size,
			    size_t * sent_size,
			    int nb_pieces, size_t pieces_size);

int startClientSocket (int port, char *name);

void sigchld_handler (int s);

int startServerSocket (int port);

/*
 *  struct that defines the type of distribution used,
 *  also used to store a string cut in two substrings
 */
struct distrib
{
  //distribution type u "uniform", g "gaussian", if relevant
  char type;
  //min value for 'u', mean value for 'g', if relevant
  char arg1[SIZE_OF_STRING];
  //max value for 'u', standard deviation value for 'g', if relevant
  char arg2[SIZE_OF_STRING];
  //other union of ranges
  struct distrib *nextDistrib;
};

/*
 *  returns an int random value of the distribution
 *  type is the type of distribuion
 *  r is a seed for generating random numiber with libgsl
 *  arg1 and arg2 arg argument defining de distribution
 */
double dist (char type, gsl_rng * r, char *arg1, char *arg2);

/* returns the distribution from de string in the script */
struct distrib *defDistrib (char *str);

int ithName (struct distrib *myDist, int i, char *tmp);
