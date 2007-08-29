/*
 *  ex_MPI.c
 *  net_MPI
 *
 *  Created by Basile Clout on 28/06/07.
 *  Copyright 2007 Basile Clout. All rights reserved.
 *
 */

#include "net_MPI.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int usage(){

	printf("\n\n");
	printf("ex_MPI uses the netMPI library to evaluate the MPI's latency and bandwidth in the parallel network.\n\n");
	printf("Usage: mpirun ex_MPI [OPTIONS]\n\n");
	printf("Examples:\n");
	printf("    mpirun ex_MPI --bsr -p network.log\n");
	printf("    # Describes the network using the bidirectional MPISend/MPIRecv method and print the output in network.log.\n\n");
	printf("Options:\n");
	printf("\t --sr\t\t Use Monodirectional MPI Send/Recv.\n");
	printf("\t --bsr\t\t Use Bidirectional MPI Send/Recv.\n");
	printf("\t --sr\t\t Use Monodirectional MPI ISend/IRecv.\n");
	printf("\t --sr\t\t Monodirectional MPI ISend/IRecv.\n");
	printf("\t --no-latency\t\t Don't take into account latency\n");
	printf("-m,\t --master \t\t Node number of the master node\n");
	printf("-c,\t --max_coefs \t\t Bit left switch defining the size of the biggest test (in bytes)\n");
	printf("-s,\t --skip	\t\t Effectively skip the s first tests\n");
	printf("-n,\t --tests \t\t Number of (effective) tests for one test size\n");
	printf("-r,\t --requests \t\t Number of parallel requests for an Asynchronous MPI ISend/IRecv.\n");
	printf("-p,\t --print \t\t Reroute the standard output\n");
	printf("-h,\t --help \t\t Print this help\n");
	printf("\n\n");

	return 1; 
}

int main(int argc, char **argv)
{
	int myrank;
	int nbtasks;

	char *filename = NULL;
	char myname[LENGTH];
	int nb_methods = NB_METHODS;
	int i, j, c, l;
	int g;
	int option_index = 0;

	int master_rank;
	int skip;
	int nb_tests;
	int isr_requests;
	int max_coefs;
	static int fl_sr, fl_bsr, fl_isr, fl_bisr;
	static int fl_lat;
	static int fl_help;

	double *latency;

	double ***arr_results;

	FILE *stream; 

	char buffer[LENGTH];
	time_t curtime;
	struct tm *loctime;

	/* DEFAULT VALUES */
	master_rank = 0;
	max_coefs = MAX_COEF_SIZE_T1;
	skip = SKIP;
	nb_tests = NB_TESTS;
	isr_requests = NB_REQUESTS;
	fl_lat = 1;
	fl_sr = fl_bsr = fl_isr = fl_bisr = 0;  
	stream = stdout;
	fl_help = 0;

	/* PARSE COMMAND ARGUMENTS */	
	static struct option long_options[] = {

		{"sr", no_argument, &fl_sr, 1},
		{"bsr", no_argument, &fl_bsr, 1},
		{"isr", no_argument, &fl_isr, 1},
		{"bisr", no_argument, &fl_bisr, 1},
		{"no-latency", no_argument, &fl_lat, 0},

		{"master", required_argument, 0, 'm'},
		{"max_coefs", required_argument, 0, 'c'},
		{"skip", required_argument, 0, 's'},
		{"tests", required_argument, 0, 'n'},
		{"requests", required_argument, 0, 'r'},
		{"print", required_argument, 0, 'p'},
		{"help", required_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while(1){

		g = getopt_long (argc, argv, "hm:c:s:n:r:p:", long_options, &option_index); 

		//	printf("g=%d, optarg=%s\n", g, optarg);
		if (g == -1)
			break;

		switch(g){

			case 0:
				if(long_options[option_index].flag != 0)
					break;
				else 
					printf("Oooooops handling long options\n");

			case 'm':
				master_rank = (int)atoi(optarg); break;

			case 'c':
				max_coefs = (int)atoi(optarg); break;

			case 's':
				skip = (int)atoi(optarg); break;

			case 'n':
				nb_tests = (int)atoi(optarg); break;

			case 'r':
				isr_requests = (int)atoi(optarg); break;

			case 'h':
				fl_help = 1;
				break;

			case 'p':
				filename = optarg;
				if((stream = fopen(filename, "a" )) == NULL)
					printf("Impossible to open %s\n", filename);
				break;

			case '?':
				break;                          /* Error in the given arguments */
		}
	}

	if(optind < argc && filename == NULL){
		filename = argv[optind];
		if((stream = fopen(filename, "a" )) == NULL)
			printf("Impossible to open %s\n", filename);
	}	

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &nbtasks);
	/*Change error handler*/
	MPI_Errhandler_set(MPI_COMM_WORLD,MPI_ERRORS_RETURN);

	/* Greetings */
	gethostname(myname,sizeof(myname));
	printf("# Hello, I am %s, my rank is %d of %d.\tNode %d is my master.\n",myname, myrank, nbtasks, master_rank);fflush(stdout);


	/* Print Help message ? */
	if(myrank == master_rank && fl_help)
		usage();

	MPI_Barrier(MPI_COMM_WORLD);

	/* Initialization of result array */

	arr_results = (double ***)malloc(nb_methods*sizeof(double **));
	for(i=0;i<nb_methods;i++){
		arr_results[i] = (double **)malloc(max_coefs*sizeof(double *));
		for(c=0;c<max_coefs;c++)
			arr_results[i][c] = (double *)calloc(nbtasks+1, sizeof(double));
	}	


	/* Determine latency */

	latency = (double *)calloc(nbtasks+1, sizeof(double));

	double l1;
	if(fl_lat==1){
		l1 -= MPI_Wtime();
		latency = net_MPI_lat_bsr(master_rank, nbtasks, myrank, nb_tests, skip); /* No skip for measuring latency */
		l1 += MPI_Wtime();
	}

	/* Loops method and message length */

	double e1, e2, e3, e4;

	if(fl_sr==1){
		e1-=MPI_Wtime();
		for(c = 0; c < max_coefs; c++){
			arr_results[0][c] = net_MPI_bdw_sr(master_rank, nbtasks, myrank, 1<<c, nb_tests, skip, latency);
		}
		e1+=MPI_Wtime();
	}

	if(fl_bsr==1){
		e2-=MPI_Wtime();
		for(c = 0; c < max_coefs; c++)
			arr_results[1][c] = net_MPI_bdw_bsr(master_rank, nbtasks, myrank, 1<<c, nb_tests, skip, latency);
		e2+=MPI_Wtime();
	}

	e3-=MPI_Wtime();
	if(fl_isr==1){
		for(c = 0; c < max_coefs; c++)
			arr_results[2][c] = net_MPI_bdw_isr(master_rank, nbtasks, myrank, 1<<c, nb_tests, skip, isr_requests, latency);
		e3+=MPI_Wtime();
	}

	if(fl_bisr==1){
		e4-=MPI_Wtime();
		for(c = 0; c < max_coefs; c++)
			arr_results[3][c] = net_MPI_bdw_bisr(master_rank, nbtasks, myrank, 1<<c, nb_tests, skip, isr_requests, latency);
		e4+=MPI_Wtime();
	}


	/* Print results */
	if(myrank==master_rank){


		fprintf(stream, "\n");
		fprintf(stream, "# Basile Clout, MPI bandwidth and latency tests\n");
		curtime = time(NULL);
		loctime = localtime(&curtime);
		fputs("# ", stream);fputs(asctime(loctime), stream);putc('\n', stream);
		fprintf(stream, " # Master node is: %s\n\n", myname);

		if(fl_lat==1){
			fprintf(stream, "# TEST LATENCY, nb_tests(eff)=%d, skip=%d\n", nb_tests, skip);
			fprintf(stream, "# TEST LATENCY last: %.2fs.\n",l1);
			fprintf(stream, "# Latency between %d and the world, in ms.\n", master_rank);
			for(i=0;i<nbtasks+1;i++)
				latency[i]*=1000;               /* Conversion in ms */
			displ_txtp(stream, &latency, 1, nbtasks, myrank);
		}

		if(fl_sr==1){
			fprintf(stream, "# TEST 1:sr, max_coefs=%d, nb_tests(eff)=%d, skip=%d\n", max_coefs, nb_tests, skip);
			fprintf(stream, "# TEST1 last: %.2fs.\n",e1);
			fprintf(stream, "# Bandwidth between %d and the world, in MBits/s\n", master_rank);
			displ_txtp(stream, arr_results[0], max_coefs, nbtasks, myrank);		
		}

		if(fl_bsr==1){
			fprintf(stream, "# TEST 2:bsr, max_coefs=%d, nb_tests(eff)=%d, skip=%d\n", max_coefs, nb_tests, skip);
			fprintf(stream, "# TEST2 last: %.2fs.\n",e2);
			fprintf(stream, "# Bandwidth between %d and the world, in MBits/s\n", master_rank);
			displ_txtp(stream, arr_results[1], max_coefs, nbtasks, myrank);		
		}

		if(fl_isr==1){
			fprintf(stream, "# TEST 3:isr, max_coefs=%d, nb_tests=(eff)%d, skip=%d, isr_requests=%d\n", max_coefs, nb_tests, skip, isr_requests);
			fprintf(stream, "# TEST3 last: %.2fs.\n",e3);
			fprintf(stream, "# Bandwidth between %d and the world, in MBits/s\n", master_rank);
			displ_txtp(stream, arr_results[2], max_coefs, nbtasks, myrank);		
		}

		if(fl_bisr==1){
			fprintf(stream, "# TEST 4:bisr, max_coefs=%d, nb_tests=(eff)%d, skip=%d, isr_requests=%d\n", max_coefs, nb_tests, skip, isr_requests);
			fprintf(stream, "# TEST4 last: %.2fs.\n",e4);
			fprintf(stream, "# Bandwidth between %d and the world, in MBits/s\n", master_rank);
			displ_txtp(stream, arr_results[3], max_coefs, nbtasks, myrank);		
		}

		if(filename!=NULL)
			printf("# File %s written.\n", filename);
	}

	MPI_Finalize();
	return 0;
}

