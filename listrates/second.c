/*
 * metarates
 * Copyright 2004, Scientific Computing Division, University Corporation  
 * for Atmospheric Research
 */

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

static long initialsec = 0;
static long initialusec = 0;

int initsec() {
	struct timeval tv;
	int ival;

	ival = gettimeofday(&tv, NULL);
        if (ival != 0) {
		fprintf(stderr, "error in call to gettimeofday!\n");
		exit(1);
	}

	initialsec  = tv.tv_sec;
	initialusec = tv.tv_usec;

	return 0;

}

double secondr()
{
	struct timeval tv;
	int ival;
	double diff;
	long diffs, diffus;

	ival = gettimeofday(&tv, NULL);
        if (ival != 0) {
		fprintf(stderr, "error in call to gettimeofday!\n");
		exit(1);
	}

        if (tv.tv_usec < initialusec) {
		tv.tv_usec += 1000000;
                tv.tv_sec -= 1;
        }
	diffs  = tv.tv_sec - initialsec;
        diffus = tv.tv_usec - initialusec;

        diff = (double)(diffs + (diffus / 1000000.));

	return (diff);
}
	
/*
int main(int argc, char** argv)
{

	initsec_();
	sleep(11);
	printf("diff = %f\n", secondr_());
}
*/

