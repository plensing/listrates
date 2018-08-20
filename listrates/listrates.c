/*
 * listrates
 * An extension of metarates working with access lists on arbitrary file sets.
 *
 * metarates
 * Copyright 2004, Scientific Computing Division, University Corporation for Atmospheric Research
 * See http://www.cisl.ucar.edu/css/software/metarates/
 */
#include "mpi.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <utime.h>
#include "listrates.h"

#define MAXPATHLENGTH 4096
char**	pname;
char    tmpName[MAXPATHLENGTH];
enum 	BenchmarkType { t_create, t_remove, t_stat, t_utime, t_data, t_data_pure, t_invalid };

main(int argc, char* argv[])
{
	int		numfiles = 0;
	int		numfilesPerProc;
	int		npes;
	int		node, npe;
	int		i, j;
	int		ival, fd, c;
	double		time0, time1, ctime, ctmax, ctmin, ctsum, ctavg, cttot;
	extern char	*optarg;
	extern int	optind, opterr, optopt;
	char 		opts[] = "l:s:CRSUDP";
	char*		progname = argv[0];
	struct stat	statbuf;
	enum BenchmarkType btype = t_invalid;
	int 		iosize = 4096;
	char*		listname;

  
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&npe);
	MPI_Comm_rank(MPI_COMM_WORLD,&node);
  
	/*
	 * get input parameters
	 */
	if ( argc <= 2 ) {
		describe(progname);
		exit(1);
	}
  
	while ( ( c = getopt(argc,argv,opts) ) != -1 ) {
		switch (c) {
		case 'l':
			listname = optarg;
			break;
		case 'C':
			btype = t_create;
			break;
		case 'R':
			btype = t_remove;
			break;
		case 'S':
			btype = t_stat;
			break;
		case 'U':
			btype = t_utime;
			break;
		case 'D':
			btype = t_data;
			break;
		case 'P':
			btype = t_data_pure;
			break;
		case 's':
			iosize = atoi(optarg);
			break;
		case '?':
		case ':':
			describe(progname);
			exit(4);
		}
	}
	if (btype == t_invalid) {
	  	perror ("Invalid benchmark type \n");
	   	MPI_Finalize();
	   	exit(1);
	}

	npes = npe;
    FILE * pFile = fopen (listname , "r");
    if (pFile == NULL) {
    	perror ("Error opening list");
    	MPI_Finalize();
    	exit(1);
    }
    numfiles = 0;
    while(fgets(tmpName, MAXPATHLENGTH, pFile))
    	numfiles++;
    fclose(pFile);

	numfilesPerProc = numfiles / npes;
	if (node == 0) {
		if ((numfilesPerProc*npes) != numfiles) {
			fprintf(stderr, "error: number of files, %d, not "
				"divisible by number of processes %d\n", 
				numfiles, npes);
			MPI_Finalize();
			exit(1);
		}
	}
  
	if (node == 0) {	
		printf("\n\nrun parameters:\n");
		printf("listname = %s\n", listname);
		printf("total number of files used in tests = %d\n", numfiles);
		printf("number of files used per process in tests = %d\n", numfilesPerProc);
		printf("Benchmark Type: ");
		switch(btype){
		case t_create:
			printf("create \n");
			break;
		case t_remove:
			printf("remove \n");
			break;
		case t_stat:
			printf("stat \n");
			break;
		case t_utime:
			printf("utime \n");
			break;
		case t_data:
			printf("data. IO size is %d Byte.\n",iosize);
			break;
		case t_data_pure:
			printf("pure data (does not include lookup time), IO size is %d Byte.\n",iosize);
			break;
		}
	}

	/*
	 * allocate space for path names
	 */
	pname = (char**)malloc(numfilesPerProc*sizeof(char*));
	if (pname == NULL) {
		perror("malloc error");
		MPI_Finalize();
		exit(1);
	}
	for (i = 0; i < numfilesPerProc; i++) 
		pname[i] = NULL;

	/*
	 * obtain file list
	 */
	getFileList(pname, node, numfilesPerProc, listname);

	/* Make sure storage does not contain garbage. */
	ctmax = ctmin = ctsum = 0;


	if (btype == t_create) {
		/*
		 * measure file creation rates
		 */
		ival  = initsec();
		MPI_Barrier(MPI_COMM_WORLD);
		time0 = secondr();
		for (i = 0; i < numfilesPerProc; i++) {
			if ((fd = creat(pname[i], 0666)) < 0) {
				fprintf(stderr, "return value = %d "
					"when trying to creat file "
					"%s.\n", fd, pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
			if ((ival = close(fd)) < 0) {
				fprintf(stderr, "return value = %d "
					"when trying to close file "
					"%s.\n", ival, pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
		}

		time1 = secondr();
		ctime = time1 - time0;

		MPI_Reduce(&ctime, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
				MPI_COMM_WORLD);

		if (node == 0) {
			ctmin = (double)(numfilesPerProc)/ctmin;
			ctmax = (double)(numfilesPerProc)/ctmax;
			ctavg = ((double)numfilesPerProc)*npes/ctsum;
			cttot = ctmax*npes;
			printf("\nrates at which files can be created "
				"and closed with %d pes:\n", npes);
			printf("max rate = %f creates per second, min rate = %f "
				"creates per second avg rate = %f creates per "
				"second, total rate = %f creates per second\n",
				ctmin, ctmax, ctavg, cttot);
    		}
	}

	if (btype == t_remove) {
		/*
		 * measure file deletion rates
		 */
		ival  = initsec();
		MPI_Barrier(MPI_COMM_WORLD);
		time0 = secondr();
		for (i = 0; i < numfilesPerProc; i++) {
			if ((ival = unlink(pname[i])) < 0) {
				fprintf(stderr, "return value "
					"= %d when trying to "
					"unlink file %s.\n",
					ival, pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
		}
		time1 = secondr();
		ctime = time1 - time0;

		MPI_Reduce(&ctime, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
				MPI_COMM_WORLD);

		if (node == 0) {
			ctmin = (double)(numfilesPerProc)/ctmin;
			ctmax = (double)(numfilesPerProc)/ctmax;
			ctavg = ((double)numfilesPerProc)*npes/ctsum;
			cttot = ctmax*npes;
			printf("\nrates at which files can be deleted with "
				"%d pes:\n", npes);
			printf("max rate = %f deletes per second, min rate = %f "
				"deletes per second avg rate = %f deletes per "
				"second, total rate = %f deletes per second\n",
				ctmin, ctmax, ctavg, cttot);
		}

	}

	if (btype == t_stat) {
		/*
		 * measure stat rates
		 */
		ival  = initsec();
		MPI_Barrier(MPI_COMM_WORLD);
		time0 = secondr();
		for (i = 0; i < numfilesPerProc; i++) {
			if ((ival = stat(pname[i], &statbuf)) < 0) {
				fprintf(stderr, "ival = %d when trying to "
					"stat a file %s.\n", ival, pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
		}
		time1 = secondr();
		ctime = time1 - time0;

		MPI_Reduce(&ctime, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
				MPI_COMM_WORLD);
		
		if (node == 0) {
			ctmin = (double)(numfilesPerProc)/ctmin;
			ctmax = (double)(numfilesPerProc)/ctmax;
			ctavg = ((double)numfilesPerProc)*npes/ctsum;
			cttot = ctmax*npes;
			printf("\nrates at which files can be stat'd with "
				"%d pes:\n", npes);
			printf("max rate = %f stats per second, min rate = %f "
				"stats per second avg rate = %f stats per "
				"second, total rate = %f stats per second\n",
				ctmin, ctmax, ctavg, cttot);
		}
	}

	if (btype == t_utime) {
		/*
		 * measure utime rates
		 */
		ival  = initsec();
		MPI_Barrier(MPI_COMM_WORLD);
		time0 = secondr();
		for (i = 0; i < numfilesPerProc; i++) {
			if ((ival = utime(pname[i], NULL)) < 0) {
				fprintf(stderr, "ival = %d when trying to "
					"utime a file %s.\n", ival, pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
		}
		time1 = secondr();
		ctime = time1 - time0;

		MPI_Reduce(&ctime, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
				MPI_COMM_WORLD);

		if (node == 0) {
			ctmin = (double)(numfilesPerProc)/ctmin;
			ctmax = (double)(numfilesPerProc)/ctmax;
			ctavg = ((double)numfilesPerProc)*npes/ctsum;
			cttot = ctmax*npes;
			printf("\nrates at which files can be utime'd with "
				"%d pes:\n", npes);
			printf("max rate = %f utimes per second, min rate = "
				"%f utimes per second, avg rate = %f utimes "
				"per second, total rate = %f utimes per "
				"second\n", ctmin, ctmax, ctavg, cttot);
		}
	}


	if (btype == t_data) {
		/*
		 * measure data access rates
		 */
		char *buffer = malloc(sizeof(char)*iosize);
		ival  = initsec();
		MPI_Barrier(MPI_COMM_WORLD);
		time0 = secondr();
		for (i = 0; i < numfilesPerProc; i++) {
			if ( (pFile	 = fopen (pname[i] , "r")) == 0) {
				free (buffer);
				fprintf(stderr, "error when trying to "
					"open a file for data read %s.\n", pname[i]);
				perror("");
				MPI_Finalize();
				exit(1);
			}
			fread (buffer,iosize,1,pFile);
			fclose(pFile);
		}
		time1 = secondr();
		ctime = time1 - time0;
		free (buffer);

		MPI_Reduce(&ctime, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
				MPI_COMM_WORLD);
		MPI_Reduce(&ctime, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
				MPI_COMM_WORLD);

		if (node == 0) {
			ctmin = (double)(numfilesPerProc)/ctmin;
			ctmax = (double)(numfilesPerProc)/ctmax;
			ctavg = ((double)numfilesPerProc)*npes/ctsum;
			cttot = ctmax*npes;
			printf("\nrates at which files can be opened, %d Byte of data read in"
					"and closed with %d pes:\n", iosize, npes);
			printf("max rate = %f reads per second, min rate = "
				"%f reads per second, avg rate = %f reads "
				"per second, total rate = %f reads per "
				"second\n", ctmin, ctmax, ctavg, cttot);
		}
	}

	if (btype == t_data_pure) {
			/*
			 * measure pure data access rates, not interested in lookup times of files
			 */
			double tmax,tmin,tsum;
			char *buffer = malloc(sizeof(char)*iosize);
			tmax = tsum = 0; tmin = 1;
			ival  = initsec();
			MPI_Barrier(MPI_COMM_WORLD);
			for (i = 0; i < numfilesPerProc; i++) {
				if ( (pFile	 = fopen (pname[i] , "r")) == 0) {
					free (buffer);
					fprintf(stderr, "error when trying to "
						"open a file for data read %s.\n", pname[i]);
					perror("");
					MPI_Finalize();
					exit(1);
				}
				time0 = secondr();
				fread (buffer,iosize,1,pFile);
				time1 = secondr();
				ctime = time1 - time0;
				fclose(pFile);

				if (ctime > tmax) tmax = ctime;
				if (ctime < tmin) tmin = ctime;
				tsum+=ctime;
			}
			free (buffer);

			MPI_Reduce(&tmax, &ctmax, 1, MPI_DOUBLE, MPI_MAX, 0,
					MPI_COMM_WORLD);
			MPI_Reduce(&tmin, &ctmin, 1, MPI_DOUBLE, MPI_MIN, 0,
					MPI_COMM_WORLD);
			MPI_Reduce(&tsum, &ctsum, 1, MPI_DOUBLE, MPI_SUM, 0,
					MPI_COMM_WORLD);


			if (node == 0) {
				ctavg = ctsum / (((double)numfilesPerProc)*npes);
				printf("\nDelays of data access (%d Byte) with %d pes:\n", iosize, npes);
				printf("max delay = %f second, min delay = %f seconds, average delay = %f seconds, "
						"total delay for all files = %f seconds\n",
						ctmax, ctmin, ctavg, ctsum);
			}
		}

	free_names(numfilesPerProc);
	MPI_Barrier(MPI_COMM_WORLD);  
	MPI_Finalize();
	return 0;
}

/*------------------------------------------------------------------------
 * describe usage of program
 *----------------------------------------------------------------------*/
void describe(char* progname)
{
	fprintf(stderr, "\t%s -l listname [-C][-R][-U][-S][-D / -P -s iosize] \n",
		progname);

	printf("Benchmark Types are: Create, Remove, Utime, Stat, Data, Pure Data.\n");
	printf("For the last two types an IO size can be specified (defaults to 4K).\n\n\n");
}

/*------------------------------------------------------------------------
 * getFileList   obtains file list for a particular process 
 *----------------------------------------------------------------------*/
void
getFileList(char** pname, int node, int numfilesPerProc,
	    char* listname)
{
	FILE * pFile = fopen (listname , "r");
	int	i;

	if (pFile == NULL) {
		perror ("Error opening list");
		MPI_Finalize();
		exit(1);
	}

	for (i = 0; i < node * numfilesPerProc; i++) {
		if(!fgets(tmpName, MAXPATHLENGTH, pFile)) {
			perror ("Error reading list");
			MPI_Finalize();
			exit(1);
		}
	}

	for (i = 0; i < numfilesPerProc; i++) {
		if(!fgets(tmpName, MAXPATHLENGTH, pFile)) {
			perror ("Error reading list");
			MPI_Finalize();
			exit(1);
		}
		tmpName[strlen(tmpName) - 1] = 0;
		if (pname[i] != NULL)
			free(pname[i]);
		pname[i] = (char*)strdup(tmpName);
		if (pname[i] == NULL) {
			perror("strdup error");
			MPI_Finalize();
			exit(1);
		}
	}
	fclose (pFile);
}


void free_names(int numNames)
{
	int i;
	for (i = 0; i < numNames; i++)
		if (pname[i] != NULL)
			free(pname[i]);
	free(pname);
}

