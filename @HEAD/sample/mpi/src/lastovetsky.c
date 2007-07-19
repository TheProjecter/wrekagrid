/* Realize the lastovetsky distribution of task on an heterogeneous platform
 * with matrix multplication as basic task */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


#define MAX(a,b) (a>b)?a:b;
#define MIN(a,b) (a<b)?a:b;

#define CONST 7
#define CLOCK(c) gettimeofday(&c,(struct timezone *)NULL)
#define CLOCK_DIFF(c1,c2) ((double)(c1.tv_sec-c2.tv_sec)+(double)(c1.tv_usec-c2.tv_usec)/1e+6)

struct proc_spec {
  int p;
  int *nbval;
  double ***val;
  long long *bounds;
};

/* compute the number of task assigned to myrank, given the size
 * of the task, the number of process and the file that contain
 * the processors performances for this problem */
int repart (int myrank, int size, int taskSize, char *file);

/* give the repartion between each node in an array */
int * optimize_with_task_size (struct proc_spec *myspec, int n);

/* give the repartion between each node in an array without caring
 * about the maximum size of each processor */
int * optimize (struct proc_spec *myspec, int n);

/* update a repartition array given the gradient of a plane curve
 * and the carateristic of the processors */
int up2date (struct proc_spec *myspec, int *x, double a);

/* generate the processors caracteristics given the data containing
 * in a file */
struct proc_spec * generate_proc_spec (char *file);

/* remove the specification of a processor in a proc_spec structure */
struct proc_spec * spec_del_rank (struct proc_spec *myspec, int rank);

void free_spec (struct proc_spec *myspec);

/* interpolate the ordinate of a point on a curve given the abscissa
 * and an array of point defining the curve */
double interpolate (int nbval, double **val, int i);

/* give the abscissa of the intersection of a curve defined by an
 * array of point and a linear curve defined by a gradient */
int intersect (int nbval, double **val, double a);

/* find the percentage limitation of the current user on the
 * current machine (need /etc/cpulim.conf) */
int findPercent (void);

/* exchnage statistics between nodes */
void make_stat (int myrank, int size, double comm_time, double sync_time, double cpu_time);

/* print the stat into a formated form */
void print_stat (double *cpu_times, double *comm_times, double *sync_times, int size);

/* realize a simple and basic task */
int doTask (unsigned long size);

/* return the time taken for synchronizing all nodes */
double sync_process (void);

/* return the time taken for computing a simulation of a
 * matrix multiplication */
double MM_task (int *data, int nbtask, int n, int taskSize);

/* return the time taken for realizing a cylic permutation of
 * the colone */
double permut_col (int *data, int n, int myrank, int size);

/* finalize the application and print an error message */
void quit (char *s);

/* Warning: takes time when taskSize > 1000 */
int main (int argc, char* argv[]) {
  int taskSize, nbtask;
  int myrank, size, MM;
  double comm_time=0., sync_time=0., cpu_time=0.;
  int i;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &myrank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  if (argc == 4) {
    taskSize = atoi (argv[argc-1]);
    MM = strcmp (argv[1], "-L");
  } else {
    printf ("Usage: %s [-MM | -L] <file_proc.txt> tasksize\n", argv[0]);
    MPI_Abort (MPI_COMM_WORLD, -1);
    exit (-1);
  }
  
  nbtask = repart (myrank, size, taskSize, argv[2]);
  
  if (MM) {
    //This simulate a matrix multiplication with cyclic colone permutation
    int *data = (int*) malloc (taskSize*(taskSize/size)*sizeof (*data));
    for (i=0; i<taskSize*(taskSize/size); i++) {
      data[i] = i;
    }
    for (i=0; i<size; i++) {
      cpu_time += MM_task (data, nbtask, (taskSize/size), taskSize);
      sync_time += sync_process ();
	printf("My rank :%d\n",myrank);
      comm_time += permut_col (data, taskSize*(taskSize/size), myrank, size);
    }
  } else {
    struct timeval c1, c2;
    CLOCK (c1);
    for (i=0; i<nbtask; i++)
      doTask (taskSize);
    CLOCK (c2);
    cpu_time = CLOCK_DIFF (c2, c1);
  }
  
  make_stat (myrank, size, comm_time, sync_time, cpu_time);
  
  MPI_Finalize ();
  return 0;
}

int repart (int myrank, int size, int taskSize, char *file) {
  int percent, *percents;
  int nbtask, *nbtasks, nmax;
  struct proc_spec * myspec;
  int i, j;

  if (!myrank) {
    percents = (int*) malloc (size*sizeof (*percents));
  }
  percent = findPercent ();
  MPI_Gather (&percent, 1, MPI_INT, percents, 1, MPI_INT, 0, MPI_COMM_WORLD);
  
  if (!myrank) {
    myspec = generate_proc_spec (file);
    if (myspec->p < size)
      quit ("Not enough proc spec");
    for (i=0; i<myspec->p; i++)
      for (j=0; j<myspec->nbval[i]; j++)
        myspec->val[i][j][1] = myspec->val[i][j][1]*percents[i]/100;
    
    for (i=0, nmax=0; i<myspec->p; i++)
      nmax += myspec->bounds[i];
    if (taskSize > nmax) {
      printf ("Max task = %d\n", nmax);
      quit (NULL);
    }
    
    nbtasks = optimize_with_task_size (myspec, taskSize);
    
    for (i=0; i<size; i++)
      printf ("proc[%d] = %d, speed = %f, time = %f\n",
	      i, nbtasks[i], interpolate (myspec->nbval[i], myspec->val[i], nbtasks[i]),
	      nbtasks[i]/interpolate (myspec->nbval[i], myspec->val[i], nbtasks[i]));
  }
  MPI_Scatter (nbtasks, 1, MPI_INT, &nbtask, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (!myrank) {
    free (percents);
    free (nbtasks);
  }
  return nbtask;
}

int * optimize_with_task_size (struct proc_spec *myspec, int n) {
  int i=0, j, rank;
  struct proc_spec *myspec_tmp;
  int *x = (int*) malloc (myspec->p * sizeof (*x));
  int *x_tmp;
  
  x_tmp = optimize (myspec, n);
  
  while (x_tmp[i] <= myspec->bounds[i]) {
    i++;
    if (i == myspec->p)
      return x_tmp;
  }
  rank = i;
  n -= myspec->bounds[rank];
  x[rank] = myspec->bounds[rank];
  
  myspec_tmp = spec_del_rank (myspec, rank);
  
  x_tmp = optimize_with_task_size (myspec_tmp, n);
  
  for (i=0, j=0; i<myspec->p; i++, j++) {
    i=(i==rank)?i+1:i;
    x[i] = x_tmp[j];
  }
  
  free (x_tmp);
  free_spec (myspec_tmp);
  return x;
}

int * optimize (struct proc_spec *myspec, int n) {
  int i, sum1, sum2, bestsum;
  int inf, sup;
  double a, besta;
  int *x = (int*) malloc (myspec->p * sizeof (*x));

  for (i=0, sum1=0, sum2=0, inf=0, sup=n, bestsum=0; i<myspec->p; i++) {
    while (1) {
      a = interpolate (myspec->nbval[i], myspec->val[i], (inf+sup)/2+1)/((inf+sup)/2+1);
      if ((sum2 = up2date (myspec, x, a)) == n) return x;
      if (sum2 < n && sum2 > bestsum) {
        bestsum = sum2;
        besta = a;
      }
      x[i] = (inf+sup) / 2;
      a = interpolate (myspec->nbval[i], myspec->val[i], x[i])/x[i];
      if ((sum1 = up2date (myspec, x, a)) == n) return x;
      if (sum1 < n && sum1 > bestsum) {
        bestsum = sum1;
	besta = a;
      }
      if (sum2 > n && sum1 < n) break;
      else if (sum1 > n) sup = x[i];
      else if (sum1 < n) inf = x[i];
    }
    if (i < myspec->p-1) {
      inf = intersect (myspec->nbval[i+1], myspec->val[i+1], a);
      sup = intersect (myspec->nbval[i+1], myspec->val[i+1], a) + (n-sum1);
    }
  }
  sum1 = up2date (myspec, x, besta);
  while (sum1<n) {
    x[sum1%myspec->p]++;
    sum1++;
  }
  return x;
}

int up2date (struct proc_spec *myspec, int *x, double a) {
  int i, sum;
  for (i=0, sum=0; i<myspec->p; i++) {
    x[i] = intersect (myspec->nbval[i], myspec->val[i], a);
    sum += x[i];
  }
  return sum;
}

struct proc_spec * generate_proc_spec (char *file) {
  struct proc_spec *result = (struct proc_spec*) malloc (sizeof (*result));
  int i, j;
  FILE *in;
  if ((in = fopen (file, "r")) == NULL)
    quit ("File absent");
  while (!feof (in) && fgetc (in) != '\n');
  if (feof (in) || fscanf (in, "p=%d\n", &(result->p)) != 1)
    quit ("Bad file format: bad processor count");
  result->val = (double***) malloc ((result->p) * sizeof (*(result->val)));
  result->nbval = (int*) malloc ((result->p) * sizeof (*(result->nbval)));
  result->bounds = (long long*) malloc ((result->p) * sizeof (*(result->bounds)));
  if (result->val == NULL || result->nbval == NULL)
    quit ("Too much processor");
  for (i=0; i<result->p; i++) {
    if (feof (in) || fscanf (in, "nbval=%d\n", result->nbval+i) != 1)
      quit ("Bad file format: bad values count");
    result->val[i] = (double**) malloc (result->nbval[i] * sizeof (**(result->val)));
    if (result->val[i] == NULL)
      quit ("Too much values");
    for (j=0; j<result->nbval[i]; j++) {
      result->val[i][j] = (double*) malloc (2 * sizeof (***(result->val)));
      if (result->val[i][j] == NULL)
        quit ("Too much values");
      if (feof (in) || fscanf (in, "%lf:%lf ", result->val[i][j], result->val[i][j]+1) != 2)
        quit ("Bad file format: bad values format");
    }
    if (feof (in) || fscanf (in, "%lld\n", result->bounds+i) != 1)
      quit ("Bad file format: bad bounds format");
  }
  fclose (in);
  return result;
}

struct proc_spec * spec_del_rank (struct proc_spec *myspec, int rank) {
  struct proc_spec *result = (struct proc_spec*) malloc (sizeof (*result));
  int i, j, k;
  result->p = myspec->p-1;
  result->val = (double***) malloc ((result->p) * sizeof (*(result->val)));
  result->nbval = (int*) malloc ((result->p) * sizeof (*(result->nbval)));
  result->bounds = (long long*) malloc ((result->p) * sizeof (*(result->bounds)));
  for (i=0, k=(i==rank)?1:0; i<result->p; ++i, k+=(i==rank)?2:1) {
    result->nbval[i] = myspec->nbval[k];
    result->val[i] = (double**) malloc (result->nbval[i] * sizeof (**(result->val)));
    for (j=0; j<result->nbval[i]; j++) {
      result->val[i][j] = (double*) malloc (2 * sizeof (***(result->val)));
      result->val[i][j][0] = myspec->val[k][j][0];
      result->val[i][j][1] = myspec->val[k][j][1];
    }
    result->bounds[i] = myspec->bounds[k];
  }
  return result;
}

void free_spec (struct proc_spec *myspec) {
  int i, j;
  for (i=0; i<myspec->p; i++) {
    for (j=0; j<myspec->nbval[i]; j++)
      free (myspec->val[i][j]);
    free (myspec->val[i]);
  }
  free (myspec->val);
  free (myspec->nbval);
  free (myspec);
}

double interpolate (int nbval, double **val, int i) {
  int j;
  int inf = 0, sup = nbval-1;
  double a, b;
  if (i>val[nbval-1][0])
    return val[nbval-1][1];
  if (i<val[0][0])
    return val[0][1];
  j = (sup+inf) / 2;
  while (i>val[j+1][0] || i<val[j][0]) {
    if (i>val[j+1][0])
      inf = j;
    else
      sup = j;
    j = (sup+inf) / 2;
  }
  a = (val[j+1][1] - val[j][1]) / (val[j+1][0] - val[j][0]);
  b = (val[j+1][0]*val[j][1] - val[j][0]*val[j+1][1]) / (val[j+1][0] - val[j][0]);
  return a*i+b;
}

int intersect (int nbval, double **val, double a) {
  int i, j;
  int inf = 0, sup = nbval-1;
  a *= 0.999999;//to avoid the approximation
  if (a<val[sup][1]/val[sup][0])
    i = nbval-1;
  else if (a>val[inf][1]/val[inf][0])
    i = -1;
  else {
    i = (inf+sup)/2;
    while (a<val[i+1][1]/val[i+1][0] || a>val[i][1]/val[i][0]) {
      if (a<val[i+1][1]/val[i+1][0])
        inf = i;
      else
        sup = i;
      i = (sup+inf) / 2;
    }
  }
  
  if (i == nbval-1) {
    j = val[i][0];
    while (a <= interpolate (nbval, val, j)/j) j++;
    return j-1;
  }
  j = val[i+1][0];
  while (a > interpolate (nbval, val, j)/j) j--;
  return j;
}

void quit (char *s) {
  if (s != NULL)
    perror (s);
  MPI_Abort (MPI_COMM_WORLD, -1);
  exit (-1);
}

int findPercent (void) {
  FILE* in;
  int percent=100;
  char uid_local[10];
  int uid = getuid ();
  if ((in = fopen ("/etc/cpulim.conf", "r")) == NULL) {
    perror ("Error while opening /etc/cpulim.conf");
    return 100;
  }
  while (!feof (in) && fscanf (in, "%d %s\n", &percent, uid_local) == 2)
    if (uid_local[0] == '*' || uid == atoi (uid_local)) {
      fclose (in);
      return percent;
    }
  return percent;
}

int doTask (unsigned long size) {
  unsigned long i, tmp;
  unsigned long *array;
  
  if ((array = (unsigned long*) malloc (size*sizeof (*array))) == NULL)
    return -1;
  
  for (i=0; i<size; i++) {
    array[i] = i;
  }

  while (*array != size-1) {
    tmp = *array;
    for (i=0; i<size-1; i++) {
      array[i] = array [i+1] * CONST;
    }
    array [size-1] = tmp * CONST;
    for (i=0; i<size; i++) {
      array[i] /= CONST;
    }
  }
  
  return 1;
}

double sync_process (void) {
  struct timeval c1, c2;
  CLOCK (c1);
  MPI_Barrier (MPI_COMM_WORLD);
  CLOCK (c2);
  return CLOCK_DIFF (c2, c1);
}

double MM_task (int *data, int nbtask, int n, int taskSize) {
  struct timeval c1, c2;
  int i, j, k;
int iValeur;
  CLOCK (c1);
  for (i=0; i<nbtask; i++) //count the line
    for (j=0; j<n; j++) //count the col
      for (k=0; k<taskSize; k++)
        //data[j*n+i] += data[i*n+j]*k+k*data[k+i+j];
	//Modif OD
	iValeur += data[i*n+j]*k+k*data[k+i+j];
  CLOCK (c2);
  return CLOCK_DIFF (c2, c1);
}

double permut_col (int *data, int n, int myrank, int size) {
  struct timeval c1, c2;
  int iError;
int i;
char strTemp[32]; 
 MPI_Status status;
  MPI_Request request1, request2;
  CLOCK (c1);
  iError = MPI_Isend (data, n, MPI_INT, (myrank+1)%size, 0, MPI_COMM_WORLD, &request1);
  MPI_Irecv (data, n, MPI_INT, (myrank+size-1)%size, 0, MPI_COMM_WORLD, &request2);
  MPI_Wait (&request1, &status);
  MPI_Wait (&request2, &status);
  CLOCK (c2); 
  return CLOCK_DIFF (c2, c1);
}

void make_stat (int myrank, int size, double comm_time, double sync_time, double cpu_time) {
  double *comm_times, *sync_times, *cpu_times;
  if (!myrank) {
    comm_times = (double*) malloc (size*sizeof (*comm_times));
    sync_times = (double*) malloc (size*sizeof (*sync_times));
    cpu_times = (double*) malloc (size*sizeof (*cpu_times));
  }
  MPI_Gather (&comm_time, 1, MPI_DOUBLE, comm_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather (&sync_time, 1, MPI_DOUBLE, sync_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather (&cpu_time, 1, MPI_DOUBLE, cpu_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (!myrank) {
    print_stat (cpu_times, comm_times, sync_times, size);
    free (comm_times); free (sync_times); free (cpu_times);
  }
}

void print_stat (double *cpu_times, double *comm_times, double *sync_times, int size) {
  int i;
  double min_time=cpu_times[0]+comm_times[0]+sync_times[0], max_time=0;
  double min_cpu=cpu_times[0], max_cpu=0;
  double min_comm=comm_times[0], max_comm=0;
  double min_sync=sync_times[0], max_sync=0;
  double mean_times = 0., mean_cpu = 0., mean_comm = 0., mean_sync = 0.;
  double *times = (double*) malloc (size*sizeof (*cpu_times));
  printf ("Proc number | effective time | cpu time | comm time | sync time + proportion\n");
  for (i=0; i<size; i++) {
    times[i] = cpu_times[i] + comm_times[i] + sync_times[i];
    min_time = (times[i]<min_time)?times[i]:min_time;
    max_time = (times[i]>max_time)?times[i]:max_time;
    min_cpu = (cpu_times[i]<min_cpu)?cpu_times[i]:min_cpu;
    max_cpu = (cpu_times[i]>max_cpu)?cpu_times[i]:max_cpu;
    min_comm = (comm_times[i]<min_comm)?comm_times[i]:min_comm;
    max_comm = (comm_times[i]>max_comm)?comm_times[i]:max_comm;
    min_sync = (sync_times[i]<min_sync)?sync_times[i]:min_sync;
    max_sync = (sync_times[i]>max_sync)?sync_times[i]:max_sync;
    printf ("%11d | %14f | %8f | %9f | %9f + %8f %%\n",
		    i, times[i], cpu_times[i], comm_times[i], sync_times[i], (sync_times[i]+comm_times[i]) * 100 / (comm_times[i]+sync_times[i]+cpu_times[i]));
    mean_times += times[i]; mean_cpu += cpu_times[i]; mean_comm += comm_times[i]; mean_sync += sync_times[i];
  } 
  mean_times /= size; mean_cpu /= size; mean_comm /= size; mean_sync /= size;
  printf ("       mean | %14f | %8f | %9f | %9f + %8f %%\n",
		  mean_times, mean_cpu, mean_comm, mean_sync, (mean_sync+mean_comm) * 100 / (mean_times));
  printf ("(max-min)/min | %12f | %8f | %9f | %9f \n", (max_time-min_time)/min_time,
		  (max_cpu-min_cpu)/min_cpu, (max_comm-min_comm)/min_comm, (max_sync-min_sync)/min_sync);
  free (times);
} 
