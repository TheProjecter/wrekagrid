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
#include <dirent.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FILE_CONF "/etc/cpulim.conf"
#define CLOCK(c) gettimeofday(&c,(struct timezone *)NULL)
#define CLOCK_UDIFF(c1,c2) ((c1.tv_sec-c2.tv_sec)*1e+6+(c1.tv_usec-c2.tv_usec))

/* TODO: interactive program should not be interrupt
 * TODO: don't control program whose name is cpulim (took this in
 * /proc/<pid>/stat)
 * Current solution: cpulim is a root program and
 * /etc/cpulim.conf have "100 0" (we can use a particular user
 * for this with the appropriate rigth), flexible */

/* One linked list for each process on the OS */
struct linked_list
{
  int pid;
  int uid;
  int flag;			// flag = 0 when a pid is found with search_pid (1 else)
  struct linked_list *next;
};

/* search the structure corresponding to the pid and give
 * the value 0 to the flag of the found structure */
int search_pid (struct linked_list *list, int pid);

/* if pid is not inside the linked list, we add it */
void add_pid (struct linked_list **list, int pid, int uid);

/* remove each pid that have these flag equal to 1 and initialized
 * all others to 1 */
void up2date_list (struct linked_list **list);

/* find the percentage limitation of the current user on the
 * current machine (need /etc/cpulim.conf) */
int findPercent (int uid);

/* handler for the termination or the program */
void hand (int sig);

int
main (int argc, char **argv)
{
  struct dirent *entry;
  struct sched_param max;
  char filename[80], line[1000], order[80], *endptr;
  FILE *pp;
  int percent;
  struct linked_list *list = NULL;
  uid_t uid;
  pid_t pid, mypid = getpid ();
  DIR *proc_dir = opendir ("/proc");

  if (proc_dir == NULL) {
    perror ("Error with the proc file-system");
    exit (-1);
  }

  max.sched_priority = sched_get_priority_max (SCHED_FIFO);
  if (sched_setscheduler (0, SCHED_FIFO, &max) == -1) {
    perror ("Was not able to acquire RT priority (however this is ok)");
  }

  if (argc == 1)
    daemon (0, 0);
  else if (!(argc == 2 && argv[1][0] == '-' && argv[1][1] == 'i')) {
    printf ("Usage: %s [-i]\n", argv[0]);
    exit (-1);
  }
  
  /* use the normal procedure for quiting */
  signal (SIGHUP, hand);
  signal (SIGINT, hand);
  signal (SIGTERM, hand);
  while (1) {
    while ((entry = readdir (proc_dir)) != NULL) {
      /* for each entry or the /proc directory, check if it is a pid */
      pid = strtol (entry->d_name, &endptr, 10);
      /* if it is not a pid, or the pid of the current program, or
       * if the pid is aleady inside the linked list: there is no
       * particular traitement */
      if (*endptr != '\0' || pid == mypid || search_pid (list, pid))
	continue;
      sprintf (filename, "/proc/%d/status", pid);
      if ((pp = fopen (filename, "r")) == NULL) {
	perror ("A pid entry disapear from the proc file-system");
	continue;
      }
      /* retreive the uid of the process */
      while (!feof (pp) && fgets (line, 1000, pp) != NULL) {
	if (sscanf (line, "Uid:\t%*d %d", &uid) == 1)
	  break;
      }
      fclose (pp);
      add_pid (&list, pid, uid);
      /* launch an instance of cpulim */
      if ((percent = findPercent (uid)) - 100) {
	//sprintf (order, "cpulim %d %d &> /dev/null &", percent, pid);
	sprintf (order, "cpulim %d %d &", percent, pid);
	printf ("%s\n", order);
	system (order);
      }
    }
    rewinddir (proc_dir);
    /* remove terminated pid that were not present in /proc */
    up2date_list (&list);
    /* check the list of pid each 100 ms */
    usleep (100000);
  }
}

int
findPercent (int uid)
{
  FILE *in;
  int percent = 100;
  char uid_local[10];
  if ((in = fopen (FILE_CONF, "r")) == NULL) {
    perror ("Configuration file does not exist, create it (with \"100 0\")");
    hand (0);
  }
  while (!feof (in) && fscanf (in, "%d %s\n", &percent, uid_local) == 2)
    if (uid_local[0] == '*' || uid == atoi (uid_local)) {
      fclose (in);
      return percent;
    }
  return percent;
}

int
search_pid (struct linked_list *list, int pid)
{
  struct linked_list *ptr;
  ptr = list;
  while (ptr != NULL && ptr->pid != pid)
    ptr = ptr->next;
  if (ptr != NULL)
    ptr->flag = 0;
  return ptr != NULL;
}

void
add_pid (struct linked_list **list, int pid, int uid)
{
  struct linked_list *ptr = (struct linked_list *) malloc (sizeof (*ptr));
  //printf ("Add pid=%d and uid=%d\n", pid, uid);
  ptr->pid = pid;
  ptr->uid = uid;
  ptr->flag = 0;
  ptr->next = *list;
  *list = ptr;
}

void
up2date_list (struct linked_list **list)
{
  struct linked_list *ptr, *ptr_prev;
  while (*list != NULL && (*list)->flag == 1) {
    ptr = *list;
    //printf ("Remove pid%d\n", ptr->pid);
    *list = (*list)->next;
    free (ptr);
  }
  if (*list == NULL)
    return;
  (*list)->flag = 1;
  ptr = *list;
  while (ptr->next != NULL) {
    ptr_prev = ptr;
    ptr = ptr->next;
    if (ptr->flag == 1) {
      //printf ("Remove pid%d\n", ptr->pid);
      ptr_prev->next = ptr->next;
      free (ptr);
      ptr = ptr_prev->next;
    } else
      ptr->flag = 1;
  }
}

void
hand (int sig)
{
  if (sig == SIGINT || sig == SIGTERM || sig == SIGHUP)
    system ("killall cpulim &> /dev/null");
  exit (1);
}
