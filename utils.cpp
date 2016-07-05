#include "utils.h"

//getting units (kibi, mebi, gibi...) from a number
char units(unsigned int n)
{ int frac;
	frac = n/1024;
	if (frac == 0) return(' ');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('K');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('M');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('G');
	printf("Error: n is too large!\n");
}


int siz(unsigned int n)
{ int frac;
	frac = n/1024;
	if (frac == 0) return(n);
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return(n);
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return(n);
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return(n);
	printf("Error: n is too large!\n");
}

inline unsigned long get_cycles()
{
	uint32_t eax = 0, edx;

	__asm__ __volatile__(
			//get informations on cpu
			"cpuid;"
			// get timestamp since last reset
			"rdtsc;"
			: "+a" (eax), "=d" (edx)
			:
			: "%rcx", "%rbx", "memory");

	__asm__ __volatile__("xorl %%eax, %%eax;"
			"cpuid;"
			:
			:
			: "%rax", "%rbx", "%rcx", "%rdx", "memory");

	return (((uint64_t)edx << 32) | eax);
}

int checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MIN(minDelta, MAX(Delta,0));
	}

   return(minDelta);
    }

double mysecond()
{
	struct timespec  ti;
        int i;

        i = clock_gettime(CLOCK_MONOTONIC, &ti);
	if ( i < 0) 
	{
		printf("error getting time\n");
		exit(EXIT_FAILURE);
       	}
 
        return ( (double) ti.tv_sec + (double) ti.tv_nsec * 1.e-9 );
}

char units_d(double f)
{ 
	long n = (long) f;
	long frac;
	frac = n/1024;
	if (frac == 0) return(' ');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('K');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('M');
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return('G');
	printf("Error: n is too large!\n");
}


double siz_d( double f)
{ 
	long n = (long) f;
	long frac;
	frac = n/1024;
	if (frac == 0) return f;
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return f/1024;
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return f/(1024 * 1024);
	n = n/1024;
	frac = n/1024;
	if (frac == 0) return f/(1024 * 1024 * 1024);
	printf("Error: n is too large!\n");
}
