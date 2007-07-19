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

#include <asm/param.h>
#include <ctype.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define UDIFF_SJ(a,b) (a*1e+6)-(b*1e+6)/HZ

pid_t pid;
long long time_tot = 0, work_time_tot = 1;

void quit (int sig);

int
main (int argc, char **argv)
{
  int percent;

  if (argc != 3) {
    printf ("Usage: %s percent pid\n", argv[0]);
    exit (-1);
  }

  percent = atoi (argv[1]);
  pid = atoi (argv[2]);
  
  if (percent > 100 || percent < 1) {
    perror ("Bad percentage");
    exit (-1);
  }

  if (percent != 100) {
    int flag = 1, utime;
    long long time_dead = 0, starttime;
    FILE *stat, *uptime;
    char filename[80], state;
    extern int errno;
    struct sched_param min;
    double second_since_begin;

    min.sched_priority = sched_get_priority_min (SCHED_FIFO);
    if (sched_setscheduler (0, SCHED_FIFO, &min) == -1) {
      perror ("Was not able to acquire RT priority (however this is ok)");
    }
    
    /* use the normal procedure for quiting */
    signal (SIGHUP, quit);
    signal (SIGINT, quit);
    signal (SIGTERM, quit);

    sprintf (filename, "/proc/%d/stat", pid);
    while (1) {
      /* read the stat in the proc file (the file manipulation
       * takes 90 us) */
      //TODO: Read the starttime only once and reduce the manipulation time
      if ((stat = fopen (filename, "r")) == NULL
	  || (uptime = fopen ("/proc/uptime", "r")) == NULL) {
	perror ("Error in the proc system");
	quit (0);
      }
      if (fscanf
	  (stat,
	   "%*d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %*d %*d %*d %*d %*d %*d %*d %lld",
	   &state, &utime, &starttime) != 3) {
	perror ("Bad stat file (in /proc)");
	quit (0);
      }
   second_since_begin=0;
       if (fscanf (uptime, "%lf", &second_since_begin) != 1) {
	perror ("Bad uptime file (in /proc)");
	quit (0);
      }
      fclose (stat);
      fclose (uptime);
      /* The time during which the program is supposed to work
       * but don't work is removed, this is dead time */
      if ((utime * 1e+6) / HZ == work_time_tot && flag)
	time_dead = UDIFF_SJ (second_since_begin, starttime) - time_tot;

     //printf("time dead = %d",time_dead); 
      time_tot = UDIFF_SJ (second_since_begin, starttime) - time_dead;
      work_time_tot = (utime * 1e+6) / HZ;
      //printf ("%d %lld %lld %lld %lld %f\n", utime, starttime, work_time_tot,
      //         time_tot, time_dead, UDIFF_SJ(second_since_begin,starttime));
      /* In function of the different times, the process is stopped or not */
      if (work_time_tot * 1e+2 > time_tot * percent) {
	if (flag) {
	  if (state == 'T' || state == 'S')
	    continue;
	  if (!kill (pid, SIGSTOP) && errno == ESRCH) {
	    printf ("Controlled program is finiched\n");
	    quit (0);
	  }
	  flag = 0;
	}
      } else if (!flag) {
	if (!kill (pid, SIGCONT) && errno == ESRCH) {
	  printf ("Controlled program is finiched\n");
	  quit (0);
	}
	flag = 1;
      }
      /* Here: some usleep, depending on the granularity
       * and the amount of cpu avalaible (ie, it takes many
       * cpu without at least one usleep */
      usleep (1);
      usleep (1);
      usleep (1);
      usleep (1);
    }
  }
  return 0;
}

void
quit (int sig)
{
  /* if the controled process was stopped and this program is stop,
   * we continue the normal execution of the process */
  kill (pid, SIGCONT);
  if (time_tot)
    printf
      ("\nCPULIM STAT:\n\tUser time: %.2fs Total time: %.2fs Percent: %.2f\n",
       work_time_tot / 1e+6, time_tot / 1e+6,
       work_time_tot * 100. / (double) time_tot);
  exit (-1);
}
