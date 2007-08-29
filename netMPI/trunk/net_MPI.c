/*
 *  net_MPI.c
 *  net_MPI
 *
 *  Created by Basile Clout on 28/06/07.
 *  Copyright 2007 Basile Clout. All rights reserved.
 *
 */

#include "net_MPI.h"


/* Handle an error from a MPI call */

void *error_handle_MPI(int error, int myrank, char *description){

	char error_string[128];
	int error_length, error_class;

	MPI_Error_class(error, &error_class);
	MPI_Error_string(error_class, error_string, &error_length);
	fprintf(stderr, "%3d at %s: error class: %s\n", myrank, description, error_string);fflush(stdout);
	MPI_Error_string(error, error_string, &error_length);
	fprintf(stderr, "%3d: error code: %s\n", myrank, error_string);fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD,error);	
}


/* AVERAGE */

double arr_avg(double *results, int nbtasks){

	int i;
	double sum;
	for (i=0;i<nbtasks;i++){
		if(results[i]>0.000001)
			sum+=results[i];
	}

	return (double)sum/(double)(nbtasks-1);
}




/* LATENCY BASED ON BIDIRECTIONAL MPI SEND-RECV METHOD */

double *net_MPI_lat_bsr(int master_rank, int nbtasks, int myrank, int nb_tests, int skip){

	int rank, i;
	int error;
	char *buffer;
	MPI_Status status;
	double elapsed_time;
	int flag=0;

	int size = 1;

	double *results = (double *)malloc((nbtasks+1)*sizeof(double));
	buffer=(char *)malloc(size*sizeof(char));

	/* Master */
	if (myrank == master_rank){

		for (rank = 0; rank < nbtasks; rank++) {

			flag=0;	
			if(rank==myrank)
				continue;

			for(i=0;i<nb_tests+skip;i++){

				if(i>skip-1 && flag==0){
					elapsed_time=-MPI_Wtime();
					flag=1;
				}

				error = MPI_Send(buffer, size, MPI_CHAR, rank, 1, MPI_COMM_WORLD);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Send master");

				error = MPI_Recv(buffer, size, MPI_CHAR, rank, 2, MPI_COMM_WORLD, &status);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Recv master");
			}

			elapsed_time += MPI_Wtime();            /* MPI_Wtime is in seconds */
			results[rank] = elapsed_time/(nb_tests*2); 
		}

		/* Last column contains the average value for the cluster*/
		results[nbtasks]=arr_avg(results, nbtasks);

	}
	else{ /* Slaves */

		for(i=0;i<nb_tests+skip;i++){

			error=MPI_Recv(buffer, size, MPI_CHAR, master_rank, 1, MPI_COMM_WORLD, &status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank,"MPI_Recv slave");

			error=MPI_Send(buffer, size, MPI_CHAR, master_rank, 2, MPI_COMM_WORLD);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Send slave");
		}
	}

	return results;
}



/* Simple MPI SEND-RECV METHOD */

double *net_MPI_bdw_sr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, double *lat){

	int rank, i;
	int error;
	char *buffer;
	MPI_Status status;
	double elapsed_time;
	int flag=0;

	double *results = (double *)malloc((nbtasks+1)*sizeof(double));
	buffer=(char *)malloc(size*sizeof(char));

	/* Master */
	if (myrank == master_rank){

		for (rank = 0; rank < nbtasks; rank++) {

			if(rank==myrank)
				continue;

			flag=0; /*Doesnt start to count time yet */
			for(i=0;i<nb_tests+skip;i++){

				if(i>skip-1 && flag==0){
					elapsed_time=-MPI_Wtime();
					flag=1;
				}

				error = MPI_Send(buffer, size, MPI_CHAR, rank, 1, MPI_COMM_WORLD);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Send master");

			}
				error = MPI_Recv(buffer, 1, MPI_CHAR, rank, 2, MPI_COMM_WORLD, &status);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Recv master");


			elapsed_time += MPI_Wtime();            /* MPI_Wtime is in seconds */
			results[rank] = ((double)size*nb_tests*8)/((elapsed_time-lat[rank])*1000000); /* The ratio is in MBits/s */
/* 			printf("DEBUG: Elapsed time = %.6fms, bytes sent = %d, size = %d, nb_tests = %d, latency = %.6fms, resulting time = %.6f\n", \
 * 					elapsed_time*1000, size*nb_tests, size, nb_tests, lat[rank], elapsed_time*1000-lat[rank]);
 * 			printf("DEBUG: %d bytes sent in %.6fs\n", size*nb_tests, elapsed_time);
 * 			printf("DEBUG (%s in %s): Bandwidth from %d to %d is %.3f MBits/s\n", __func__, __FILE__, myrank, rank, results[rank]);
 * 			printf("\n");
 * 
 */


		}

		/* Last column contains the average value for the cluster*/
		results[nbtasks]=arr_avg(results, nbtasks);

	}
	else{ /* Slaves */

		for(i=0;i<nb_tests+skip;i++){

			error=MPI_Recv(buffer, size, MPI_CHAR, master_rank, 1, MPI_COMM_WORLD, &status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank,"MPI_Recv slave");

		}
			error=MPI_Send(buffer, 1, MPI_CHAR, master_rank, 2, MPI_COMM_WORLD);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Send slave");
	}

	return results;
}


/* Bidirectional MPI SEND-RECV METHOD */

double *net_MPI_bdw_bsr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, double *lat){

	int rank, i;
	int error;
	char *buffer;
	MPI_Status status;
	double elapsed_time;
	int flag=0;

	double *results = (double *)malloc((nbtasks+1)*sizeof(double));
	buffer=(char *)malloc(size*sizeof(char));

	/* Master */
	if (myrank == master_rank){

		for (rank = 0; rank < nbtasks; rank++) {

			flag=0;	
			if(rank==myrank)
				continue;

			for(i=0;i<nb_tests+skip;i++){

				if(i>skip-1 && flag==0){
					elapsed_time=-MPI_Wtime();
					flag=1;
				}

				error = MPI_Send(buffer, size, MPI_CHAR, rank, 1, MPI_COMM_WORLD);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Send master");

				error = MPI_Recv(buffer, size, MPI_CHAR, rank, 2, MPI_COMM_WORLD, &status);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Recv master");
			}

			elapsed_time += MPI_Wtime();            /* MPI_Wtime is in seconds */
			results[rank] = ((double)size*nb_tests*8*2)/(elapsed_time*1000000); /* The ratio is in MBits/s */
/* 			results[rank] = ((double)size*nb_tests*8*2)/((elapsed_time-nb_tests*2*lat[rank])*1000000); /* The ratio is in MBits/s */

/* 			printf("DEBUG (%s in %s): Bandwidth from %d to %d is %.3f MBits/s\n", __func__, __FILE__, myrank, rank, results[rank]);
 */
		}

		/* Last column contains the average value for the cluster*/
		results[nbtasks]=arr_avg(results, nbtasks);

	}
	else{ /* Slaves */

		for(i=0;i<nb_tests+skip;i++){

			error=MPI_Recv(buffer, size, MPI_CHAR, master_rank, 1, MPI_COMM_WORLD, &status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank,"MPI_Recv slave");

			error=MPI_Send(buffer, size, MPI_CHAR, master_rank, 2, MPI_COMM_WORLD);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Send slave");
		}
	}

	return results;
}


/* BIDIRECTIONAL MPI ISEND_IRECV METHOD */ /* idea from osu_bibw.c, mvapich */

double *net_MPI_bdw_bisr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, int nb_requests, double *lat){

	int rank,i,j;
	int error;
	char *buffer_s, *buffer_r;
	MPI_Request requests_s[NB_REQUESTS];
	MPI_Request requests_r[NB_REQUESTS];
	MPI_Status status[NB_REQUESTS];
	double elapsed_time;
	int flag=0;

	double *results = (double *)malloc((nbtasks+1)*sizeof(double));
	buffer_s=(char *)malloc(size*sizeof(char));
	buffer_r=(char *)malloc(size*sizeof(char));

	/* Master */
	if (myrank == master_rank){

		for (rank = 0; rank < nbtasks; rank++) {

			if(rank==myrank)
				continue;

			flag=0;
			for(i=0;i<nb_tests+skip;i++){

				if(i>skip-1 && flag==0){
					elapsed_time=-MPI_Wtime();
					flag=1;
				}

				for(j=0;j<nb_requests;j++){
					error = MPI_Irecv(buffer_r, size, MPI_CHAR, rank, 2, MPI_COMM_WORLD, requests_r+j);
					if(error!=MPI_SUCCESS) 
						error_handle_MPI(error, myrank, "MPI_Irecv master");
				}

				for(j=0;j<nb_requests;j++){
					error = MPI_Isend(buffer_s, size, MPI_CHAR, rank, 1, MPI_COMM_WORLD, requests_s+j);
					if(error!=MPI_SUCCESS) 
						error_handle_MPI(error, myrank, "MPI_Send master");
				}

				MPI_Waitall(nb_requests, requests_s, status);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank, "MPI_Waitall Isend master");
				MPI_Waitall(nb_requests, requests_r, status);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank, "MPI_Waitall Irecv master");
			}

			elapsed_time += MPI_Wtime();            /* MPI_Wtime is in seconds */
			results[rank] = ((double)size*nb_tests*8*nb_requests)/(elapsed_time*1000000); /* The ratio is in MBits/s */
/*  			printf("DEBUG (%s in %s): Bandwidth (%d bytes) between %d and %d is %.3f MBits/s\n", __func__, __FILE__, size, myrank, rank, results[rank]);
 */
 
		}

		/* Last column contains the average value for the cluster*/
		results[nbtasks]=arr_avg(results, nbtasks);

	}
	else{ /* Slaves */

		for(i=0;i<nb_tests+skip;i++){

			for(j=0;j<nb_requests;j++){	
				error=MPI_Irecv(buffer_r, size, MPI_CHAR, master_rank, 1, MPI_COMM_WORLD, requests_r+j);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank,"MPI_Irecv slave");
			}
			for(j=0;j<nb_requests;j++){	
				error=MPI_Isend(buffer_s, size, MPI_CHAR, master_rank, 2, MPI_COMM_WORLD, requests_s+j);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank,"MPI_Isend slave");
			}

			error=MPI_Waitall(nb_requests, requests_s, status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Waitall send slave");
			error=MPI_Waitall(nb_requests, requests_r, status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Waitall recv slave");

		}
	}

	return results;
}

/* SIMPLE MPI ISEND_IRECV METHOD */ /* idea from osu_bw.c, mvapich */

double *net_MPI_bdw_isr(int master_rank, int nbtasks, int myrank, int size, int nb_tests, int skip, int nb_requests, double *lat){

	int rank,i,j;
	int error;
	char *buffer;
	MPI_Request requests[NB_REQUESTS];
	MPI_Status status[NB_REQUESTS];
	double elapsed_time;
	int flag=0;

	double *results = (double *)malloc((nbtasks+1)*sizeof(double));
	buffer=(char *)malloc(size*sizeof(char));

	/* Master */
	if (myrank == master_rank){

		for (rank = 0; rank < nbtasks; rank++) {

			flag=0;
			if(rank==myrank)
				continue;

			for(i=0;i<nb_tests+skip;i++){

				if(i>skip-1 && flag==0){
					elapsed_time=-MPI_Wtime();
					flag=1;
				}

				for(j=0;j<nb_requests;j++){
					error = MPI_Isend(buffer, size, MPI_CHAR, rank, 1, MPI_COMM_WORLD, requests+j);
					if(error!=MPI_SUCCESS) 
						error_handle_MPI(error, myrank, "MPI_Send master");
				}

				MPI_Waitall(nb_requests, requests, status);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank, "MPI_Waitall master");

				error = MPI_Recv(buffer, 1, MPI_CHAR, rank, 2, MPI_COMM_WORLD, &status[0]);
				if(error!=MPI_SUCCESS) 
					error_handle_MPI(error, myrank, "MPI_Recv master");

			}

			elapsed_time += MPI_Wtime();            /* MPI_Wtime is in seconds */
			results[rank] = ((double)size*nb_tests*8*nb_requests)/((elapsed_time-lat[rank])*1000000); /* The ratio is in MBits/s */
/* 			printf("DEBUG (%s in %s): Bandwidth (%d bytes) from %d to %d is %.3f MBits/s\n", __func__, __FILE__, size, myrank, rank, results[rank]);
 * 
 */
		}
		
		/* Last column contains the average value for the cluster*/
		results[nbtasks]=arr_avg(results, nbtasks);
	}
	else{ /* Slaves */

		for(i=0;i<nb_tests+skip;i++){

			for(j=0;j<nb_requests;j++){	
				error=MPI_Irecv(buffer, size, MPI_CHAR, master_rank, 1, MPI_COMM_WORLD, requests+j);
				if(error!=MPI_SUCCESS)
					error_handle_MPI(error, myrank,"MPI_Irecv slave");
			}

			error=MPI_Waitall(nb_requests, requests, status);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Waitall slave");

			error=MPI_Send(buffer, 1, MPI_CHAR, master_rank, 2, MPI_COMM_WORLD);
			if(error!=MPI_SUCCESS)
				error_handle_MPI(error, myrank, "MPI_Send slave");
		}
	}

	return results;
}

/* TEXT DISPLAY RESULT */

int displ_txt(FILE *stream, double **results, int max_coef, int nbtasks, int master_rank){

	int i, c;

	/* TEST */

	/* Summary */
	fprintf(stream, "# -----------------------------------------------------------------------------\n");

	/* Table caption */
	fprintf(stream, "# buffer (bytes)\t"); 
	for(i=0;i<nbtasks;i++){ 
		if (i!=master_rank) 
			fprintf(stream, "to %d\t\t", i); 
	} 
	fputs("avg", stream);
	fprintf(stream, "\n");

	/* Table content */
	for(c=0;c<max_coef;c++){
		fprintf(stream, "  %d \t\t", 1<<c);
		for(i=0;i<nbtasks+1;i++)
			if (i!=master_rank)
				fprintf(stream, "%.3f\t\t", results[c][i]);
		fprintf(stream, "\n");
	}

	fprintf(stream,"\n\n");


	return 1;
}

/* TEXT DISPLAY RESULT PRINTING */

int displ_txtp(FILE *stream, double **results, int min_coefs, int coefs, int nbtasks, int master_rank){

	int i, c;

	/* TEST */

	/* Summary */
	fprintf(stream, "# -----------------------------------------------------------------------------\n");

	/* Table caption */
	fprintf(stream, "# buf\t"); 
	for(i=0;i<nbtasks;i++){ 
		if (i!=master_rank) 
			fprintf(stream, "to %d\t", i); 
	} 
	fputs("avg", stream);
	fprintf(stream, "\n");

	/* Table content */
	for(c=0;c<coefs;c++){
		fprintf(stream, "  %d \t", 1<<(c+min_coefs));
		for(i=0;i<nbtasks+1;i++)
			if (i!=master_rank)
				fprintf(stream, "%.3f\t", results[c][i]);
		fprintf(stream, "\n");
	}

	fprintf(stream,"\n\n");


	return 1;
}


