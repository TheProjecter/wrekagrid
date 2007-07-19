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

#include <arpa/inet.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "utils.h"
#include "builder.h"
#include "timings.h"

/**
 * \brief function get local IP address
 *
 * \param none
 * \return char* Local ip address
 * \see ---
 *
 */
char *getip ()
{
  char buffer[256];
  struct hostent *h;

  gethostname (buffer, 256);
  h = gethostbyname (buffer);

  return inet_ntoa (*(struct in_addr *) h->h_addr);
}

/**
 * \brief function to read data from a socket and log errors to a file
 *
 * \param fd handle of socket
 * \param buf pointer to buffer
 * \param count size to receive
 * \param line number of line
 * \param filename name of file to log errors
 * \return ssize_t size of data read
 * \see ---
 *
 */
ssize_t my_read (int fd, void *buf, size_t count, int line, char *filename)
{
  ssize_t sum = 0, nb = 0;
  do {
    nb = read (fd, buf + sum, count - sum);
    if (nb < 0) {
      fprintf (stderr, "read error line %d of file %s: %s\n", line, filename,
	       strerror (errno));
      return nb;
    } else if (nb == 0)		/* connection closed by peer */
      return sum;
    // printf("sum = %d, nb=%d\n",sum,nb);
    sum += nb;
  } while (sum < count);
  return sum;
}

/**
 * \brief function to write data to a socket and log errors to a file
 *
 * \param fd handle of socket
 * \param buf Buffre to write to socket
 * \param count size of buffer
 * \param line Number of line
 * \param filename name of file to log errors
 * \return ssize_t size of data writed to socket
 * \see ---
 *
 */
ssize_t my_write (int fd, void *buf, size_t count, int line, char *filename)
{
  ssize_t nb;
  nb = write (fd, buf, count);
  if (nb < 0) {
    fprintf (stderr, "write error line %d of file %s : %s\n", line, filename,
	     strerror (errno));
    fprintf (stderr, "count=%d, writen=%d\n", (int) count, (int) nb);
    perror ("write");
  }
  return nb;
}

/**
 * \brief function ...
 *
 * \param socket
 * \param data_buffer
 * \param data_size
 * \param sent_size
 * \param nb_pieces
 * \param pieces_size
 * \return ssize_t
 * \see ---
 *
 */
ssize_t raw_send_in_pieces (int socket, unsigned char *data_buffer, size_t data_size,
		    size_t * sent_size, int nb_pieces, size_t pieces_size)
{
  //uint32_t sval;
  //int8_t c;
  int i;
  for (i = 0; i < nb_pieces; i++) {
    *sent_size +=
      my_write (socket, data_buffer + i * pieces_size, pieces_size, __LINE__,
		__FILE__);
  }

  return *sent_size;
}

/**
 * \brief function to start server socket
 *
 * \param port port for socket to listen
 * \return int 0 if success
 * \see ---
 *
 */
int startServerSocket (int port)
{
  int sockfd;
  int yes = 1;
  struct sockaddr_in my_addr;	// my address information

  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
    perror ("socket");
    exit (1);
  }

  if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1) {
    perror ("setsockopt");
    exit (1);
  }

  my_addr.sin_family = AF_INET;	// host byte order
  my_addr.sin_port = htons (port);	// short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY;	// automatically fill with my IP
  memset (&(my_addr.sin_zero), '\0', 8);	// zero the rest of the struct

  if (bind (sockfd, (struct sockaddr *) &my_addr, sizeof (struct sockaddr)) ==
      -1) {
    perror ("bind");
    exit (1);
  }

  if (listen (sockfd, BACKLOG) == -1) {
    perror ("listen");
    exit (1);
  }
  return sockfd;
}

/**
 * \brief function to start client socket
 *
 * \param port port to connect to
 * \param name Name of client
 * \return int 0 if success
 * \see ---
 *
 */
int startClientSocket (int port, char *name)
{
  struct hostent *he;
  struct sockaddr_in their_addr;	// connector's address information
  int sockfd;

  if ((he = gethostbyname (name)) == NULL) {	// get the host info
    perror ("Error gethostbyname");
    exit (-4);
  }

  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
    perror ("Error when building socket");
    exit (-5);
  }

  their_addr.sin_family = AF_INET;	// host byte order
  their_addr.sin_port = htons (port);	// short, network byte order
  their_addr.sin_addr = *((struct in_addr *) he->h_addr);
  memset (&(their_addr.sin_zero), '\0', 8);	// zero the rest of the struct

  if (connect
      (sockfd, (struct sockaddr *) &their_addr,
       sizeof (struct sockaddr)) == -1) {
    perror ("Error connection socket");
    exit (-6);
  }
  return sockfd;
}

/**
 * \brief function to get value from distribution
 *
 * \param type Type of distribution
 * \param r Value to randomize
 * \param arg1 argument 1 of distribution
 * \param arg2 argument 2 of distribution
 * \return double value of distribution
 * \see ---
 *
 */
double dist (char type, gsl_rng * r, char *arg1, char *arg2)
{
/* returns an int random value of the distribution
 * type is the type of distribuion
 * r is a seed for generating random number with libgsl
 * arg1 and arg2 are arguments defining the distribution */
  switch (type) {
  case ('u'):
    return max (0,
		atof (arg1) +
		(double) ((strtod (arg2, NULL) - strtod (arg1, NULL) +
			1.0) * rand () / (RAND_MAX + 1.0)));
    break;
  case ('g'):
    return max (0, atof (arg1) + gsl_ran_gaussian (r, strtod (arg2, NULL)));
    break;
  default:
    return -1;
  }
}

/**
 * \brief function to build distribution from string
 *
 * \param str String that contains distribution
 * \return struct distrib Structure that contains parameters from distribution
 * \see ---
 *
 */
struct distrib *
defDistrib (char *str)
{
/* returns the distribution from de string in the script */
  struct distrib *tmp = (struct distrib *) malloc (sizeof (*tmp));
  int j, i = 1;
  //  printf("In defDistrib1: parameter str= %s",str);fflush(stdout);
  while (str[i] != '-' && str[i] != ';') {
    tmp->arg1[i - 1] = str[i];
    i++;
   /* printf("i=%d, str[i]=%c",i,str[i]);*/
  }
   // printf("In defDistrib2");fflush(stdout);
  tmp->arg1[i - 1] = '\0';
  //printf("arg1 : %s\n",tmp->arg1);
  if (str[i] == ';')
    tmp->type = 'g';
  else
    tmp->type = 'u';
  j = i++;
  while (str[i] != ']') {
    tmp->arg2[i - j - 1] = str[i];
    i++;
  }
    //printf("In defDistrib3");fflush(stdout);
  tmp->arg2[i - j - 1] = '\0';
  //printf("arg2 : %s\n",tmp->arg2);

  if (str[i + 1] == '-')
    tmp->nextDistrib = defDistrib (str + i + 2);
  else
    tmp->nextDistrib = NULL;

   // printf("In defDistrib4");fflush(stdout);
  return tmp;
}

/**
 * \brief function to get IP address from distribution
 *
 * \param myDist Distribution of IP address
 * \param i Number if IP
 * \param tmp IP address
 * \return int 1 if success
 * \see ---
 *
 */
int ithName (struct distrib *myDist, int i, char *tmp)
{
  char strdeb[SIZE_OF_STRING] = "\0";
  char strfin[SIZE_OF_STRING] = "\0";
  char common[SIZE_OF_STRING];
  int j, k = 0;
  int ip1[4] = { -1, -1, -1, -1 };
  int ip2[4] = { -1, -1, -1, -1 };
  int deb, fin;
  char *arg1;
  char *arg2;

  if (myDist == NULL)
    return 0;

  arg1 = myDist->arg1;
  arg2 = myDist->arg2;

  while (arg1[k] == arg2[k] && arg1[k] != '\0') {
    common[k] = arg1[k];
    k++;
  }
  k--;
  while (common[k] != '.') {
    k--;
  }
  k++;
  common[k] = '\0';
  for (j = k; *(arg1 + j); j++)
    strdeb[j - k] = arg1[j];
  for (j = k; *(arg2 + j); j++)
    strfin[j - k] = arg2[j];
  if ((strchr (strdeb, '.') == NULL) && (strchr (strfin, '.') == NULL)) {
    deb = atoi (strdeb);
    fin = atoi (strfin);
    if (fin - deb < 0) {
      printf ("Range badly formed\n");
      return 0;
    } else if (deb + i <= fin)
      sprintf (tmp, "%s%i", common, deb + i);
    else
      return ithName (myDist->nextDistrib, i - (fin - deb + 1), tmp);
  } else {
    int calc;
    //printf("il y a un point\n");
    k = 0;
    while (!((strchr (strdeb, '.') == NULL)
	     && (strchr (strfin, '.') == NULL))) {
      for (j = k; j > 0; j--) {
	ip1[j] = ip1[j - 1];
	ip2[j] = ip2[j - 1];
      }
      ip1[0] = atoi (strdeb);
      ip2[0] = atoi (strfin);
      //printf("ip tout chaud pour k = %i : %i %i\n",k,ip1[k],ip2[k]);
      if (strchr (strdeb, '.') != NULL)
	strcpy (strdeb, strchr (strdeb, '.') + 1);
      if (strchr (strfin, '.') != NULL)
	strcpy (strfin, strchr (strfin, '.') + 1);
      k++;
    }
    for (j = k; j > 0; j--) {
      ip1[j] = ip1[j - 1];
      ip2[j] = ip2[j - 1];
    }
    ip1[0] = atoi (strdeb);
    ip2[0] = atoi (strfin);

    k = 3;
    while (!(strchr (common, '.') == NULL)) {
      ip1[k] = atoi (common);
      ip2[k] = atoi (common);
      if (strchr (common, '.') != NULL)
	strcpy (common, strchr (common, '.') + 1);
      k--;
    }
    calc =
      (ip2[0] - ip1[0]) + 256 * (ip2[1] - ip1[1]) + 256 * 256 * (ip2[2] -
								 ip1[2]) +
      256 * 256 * 256 * (ip2[3] - ip1[3]);
    if (calc < 0) {
      printf ("Range badly formed or too wide\n");
      return 0;
    } else if (calc < i) {
      return ithName (myDist->nextDistrib, i - (calc + 1), tmp);
    } else {
      char buff[SIZE_OF_STRING];
      sprintf (tmp, "%d", 1 + (ip1[0] + i - 2) % 255);
      strcpy (buff, tmp);
      sprintf (tmp, "%d.%s", (ip1[1] + ((ip1[0] + i - 1) / 256)) % 256, buff);
      strcpy (buff, tmp);
      sprintf (tmp, "%d.%s",
	       (ip1[2] + ((ip1[0] + 256 * ip1[1] + i - 1) / (256 * 256))) % 256,
	       buff);
      strcpy (buff, tmp);
      sprintf (tmp, "%d.%s",
	       (ip1[3] +
		((ip1[0] + 256 * ip1[1] + 256 * 256 * ip1[2] + i -
		  1) / (256 * 256 * 256))) % 256, buff);
    }
  }

  return 1;
}
