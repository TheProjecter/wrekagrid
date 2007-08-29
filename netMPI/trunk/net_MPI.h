/*
 *  net_MPI.h
 *  net_MPI
 *
 *  Created by Basile Clout on 28/06/07.
 *  Copyright 2007 Basile Clout. All rights reserved.
 *
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>	
#include <time.h>	
#include <getopt.h>

#define LENGTH 256                              /* length of strings */
#define NB_METHODS 4                               /* Number of different methods to determine bandwidth */
 

#define MAX_COEF_SIZE_T1 22                     /* Maximum value of the test sizes (Number of bit switchs) */
#define MAX_COEF_SIZE_T2 22
#define MAX_COEF_SIZE_T3 22 
#define MAX_COEF_SIZE_T4 22
#define MIN_COEF_SIZE 0

#define SKIP 3                                  /* Number of skiped tests */
#define NB_TESTS 10                             /* Number of TOTAL tests (REAL tests = TOTAL-SKIP) */
#define NB_REQUESTS 16                          /* Number of "threads" for isr functions */


void *error_handle_MPI(int error, int myrank, char *description);
double *net_MPI_bdw_sr(int master_rank, int myrank, int nbtasks, int size, int nb_tests, int skip, double *latency);
double *net_MPI_bdw_bsr(int master_rank, int myrank, int nbtasks, int size, int nb_tests, int skip, double *latency);
double *net_MPI_bdw_isr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, int nb_requests, double *latency);
double *net_MPI_bdw_bisr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, int nb_requests, double *latency);

int displ_txt(FILE *stream, double **results, int max_coef, int nbtasks, int master_rank);
int displ_txtp(FILE *stream, double **results, int min_coef, int coefs,  int nbtasks, int master_rank);

double *net_MPI_lat_bsr(int master_rank, int nbtasks, int myrank, int nb_tests, int skip);
