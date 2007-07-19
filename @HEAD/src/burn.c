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

/* Need root rights to have priority */

#include <asm/param.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define GRANULARITY_DEFAULT 50;

#define CLOCK(c) gettimeofday(&c,(struct timezone *)NULL)
#define CLOCK_UDIFF(c1,c2) ((c1.tv_sec-c2.tv_sec)*1e+6+(c1.tv_usec-c2.tv_usec))

struct data
{
  int proc;
  int percent;
  int granularity;
  int sync;
  char *flag;
};

void *follow (void *q);

int
main (int argc, char *argv[])
{
  pthread_t *th;
  struct data *dat;
  long nbcpu = sysconf (_SC_NPROCESSORS_CONF);
  char flag;
  int i;
  int percent, granularity, desync;
  struct sched_param max;
  unsigned long mask = 1;
  unsigned int len = sizeof (mask);
  struct timeval c0, c1, c2, c3;
  long long time_tot, sleep_time_tot;

  if (argc < 2 || argc > 4) {
    printf ("Usage: %s [-desync] <percent> [<granularity>]\n", argv[0]);
    return -1;
  }

  desync = !strcmp (argv[1], "-desync");
  if (desync)
    argv++;
  percent = atoi (argv[1]);
  granularity = ((argc == 4 && desync)
		 || (argc == 3
		     && !desync)) ? atoi (argv[2]) : GRANULARITY_DEFAULT;

  daemon (0, 0);

  /* Get the max priority to be the more accurate */
  max.sched_priority = sched_get_priority_max (SCHED_FIFO);
  if (sched_setscheduler (0, SCHED_FIFO, &max) == -1) {
    perror ("Prioritizing the burn program did not succeed");
  }

  if (sched_setaffinity (0, len, &mask) < 0) {
    perror
      ("Specifying the processor on which the burn program is executed did not succeed");
  }

  th = (pthread_t *) malloc (nbcpu * sizeof (*th));
  dat = (struct data *) malloc (nbcpu * sizeof (*dat));
  for (i = 1, flag = 0; i < nbcpu; i++) {
    (dat + i)->proc = i;
    (dat + i)->percent = percent;
    (dat + i)->granularity = granularity;
    (dat + i)->sync = !desync;
    (dat + i)->flag = &flag;
    pthread_create (th + i, NULL, follow, dat + i);
  }

  /* Alternate sleep period where other process can be executed
   * and period where cpu ressource is taken */
  CLOCK (c0);
  sleep_time_tot = 0;
  while (1) {
    flag = 0;
    CLOCK (c1);
    for (i = 0; i < granularity; i++)
      usleep (1);
    flag = 1;
    CLOCK (c2);
    sleep_time_tot += CLOCK_UDIFF (c2, c1);
    while (time_tot * (100 - percent) / 100 < sleep_time_tot) {
      sched_yield ();
      CLOCK (c3);
      time_tot = CLOCK_UDIFF (c3, c0);
    }
  }

  return 0;
}

void *
follow (void *q)
{
  struct data *dat = (struct data *) q;
  unsigned long mask = pow (2, dat->proc);
  unsigned int len = sizeof (mask);
  int i;

  /* Specify the processor on which the program will take cpu time */
  if (sched_setaffinity (0, len, &mask) < 0) {
    perror
      ("Specifying the processor on which the burn program is executed did not succeed");
  }

  /* Alternate sleep period where other process can be executed
   * and period where cpu ressource is taken */
  if (dat->sync) {
    while (1) {
      for (i = 0; i < dat->granularity; i++)
	usleep (1);
      while (!*(dat->flag));
      while (*(dat->flag))
	sched_yield ();
    }
  } else {
    struct timeval c0, c1, c2, c3;
    long long time_tot, sleep_time_tot;
    CLOCK (c0);
    sleep_time_tot = 0;
    while (1) {
      CLOCK (c1);
      for (i = 0; i < dat->granularity; i++)
	usleep (1);
      CLOCK (c2);
      sleep_time_tot += CLOCK_UDIFF (c2, c1);
      while (time_tot * (100 - dat->percent) / 100 < sleep_time_tot) {
	sched_yield ();
	CLOCK (c3);
	time_tot = CLOCK_UDIFF (c3, c0);
      }
    }
  }

  return (NULL);
}
