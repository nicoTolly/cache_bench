
/* A benchmark based on STREAM
 * to saturate bandwidth between 
 * memory and cache, with several
 * threads doing vload.
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>
#include <omp.h>
#include <unistd.h>
#include <stdbool.h>
#include <immintrin.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

# define N	(1<<23)
# define K	(1<<10)
# define NTIMES	10
# define OFFSET	0

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

#define HASWELL 
// Vectorial operations 
#ifdef HASWELL
#define VLOAD _mm256_load_pd
#define VSTORE _mm256_store_pd
#define VSETZERO _mm256_setzero_pd
#define VSETHALPH _mm256_set_pd(0.5, 0.5, 0.5, 0.5)
#define VADD _mm256_add_pd
#define VTYPE __m256d
#define BYTES_PERWORD 8
#define BYTES_PERDOUBLE BYTES_PERWORD 
#define WORDS_PERVECTOR 4
#define BYTES_PERVECTOR WORDS_PERVECTOR*BYTES_PERWORD
#define VECTORS_PERLINE 2
#define BYTES_PERLINE VECTORS_PERLINE*BYTES_PERVECTOR
#define WORDS_PERLINE WORDS_PERVECTOR*VECTORS_PERLINE
#else
other architectures not handled
#endif

unsigned long cb; // size of the chunk (in bytes)
VTYPE trash;



static double	avgtime = 0, maxtime = 0,
		mintime = FLT_MAX, diffmax = 0, maxratio = 0;

static unsigned long avgcycles = 0, maxcycles = 0,
		mincycles = ULONG_MAX;


static unsigned int	bytes[1];

double * arr;

int nbvloads= 0;

double mysecond(void);
int checktick(void) ;
void load(double const *restrict a, int size, intptr_t k);
void __attribute__((optimize("O0"))) load_asm(double const * t, intptr_t n, intptr_t k) ;
double * align_ptr(double * pt, int n);
static inline unsigned long get_cycles();
char units(unsigned int n);
int siz(unsigned int n);
char units_d(double f);
double siz_d( double f);



int main()
{
	int			quantum, checktick();
	int			BytesPerWord, nTh;
	register int	 k;
	double		  times[NTIMES];
	unsigned long 	  cycles[NTIMES];


	double maxtimeTh[NTIMES];
	double mintimeTh[NTIMES];
	double maxdiff[NTIMES];


	/* --- SETUP --- determine precision and check timing --- */

	printf(HLINE);
	BytesPerWord = sizeof(double);
	printf("This system uses %d bytes per DOUBLE PRECISION word.\n",
			BytesPerWord);

	printf(HLINE);

	if  ( (quantum = checktick()) >= 1) 
		printf("Your clock granularity/precision appears to be "
				"%d microseconds.\n", quantum);
	else
		printf("Your clock granularity appears to be "
				"less than one microsecond.\n");


#pragma omp parallel
	{
		nTh = omp_get_num_threads();
	}
	double		  timesPerTh[NTIMES][nTh];

	printf("Lauching %d threads\n", nTh);
	arr = malloc(sizeof(double)*(N*nTh+4));

	//aligning arr
	double * oldarr = arr;
	arr = align_ptr(arr, 32);


	bytes[0]= nTh * N * sizeof(double);
	printf("N = %d, %d threads will be called, loading %d%c bytes of data %d times\n", N, nTh, siz(bytes[0]), units(bytes[0]), K);
	printf(HLINE);

	//initialization
#pragma omp for
	for(int i = 0; i < nTh; i++)
	{
		for (int j = 0; j < N; j++)
			arr[i*N+j]= 3.0;
	}



	printf(HLINE);
	for (k=0; k<NTIMES; k++)
	{
		times[k] = mysecond();
		cycles[k] = get_cycles();
#pragma omp parallel shared( arr , timesPerTh )
		{
			int i = omp_get_thread_num();
			timesPerTh[k][i]=mysecond();
			//load data from a section of N doubles of arr, do it K times
			load_asm(arr+i*N, N, K);
			//load(arr+i*N, N, K);

			timesPerTh[k][i] = mysecond() - timesPerTh[k][i];
		}

		times[k] = mysecond() - times[k];
		cycles[k] = get_cycles() - cycles[k];
		maxtimeTh[k]= 0;
		mintimeTh[k]= FLT_MAX;
		for (int i =0; i < nTh; i++)
		{
			maxtimeTh[k]= MAX(maxtimeTh[k], timesPerTh[k][i]);
			mintimeTh[k]= MIN(mintimeTh[k], timesPerTh[k][i]);
		}
		maxdiff[k]= maxtimeTh[k] - mintimeTh[k];
		for (int i =0; i<nTh; i++)
		{
			printf("thread %d took %f s to execute\n", i, timesPerTh[k][i]);
		}
		printf(HLINE);
	}

	/*	--- SUMMARY --- */

	double ratio;
	for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{

		avgtime = avgtime + times[k];
		mintime = MIN(mintime, times[k]);
		maxtime = MAX(maxtime, times[k]);

		avgcycles = avgcycles + cycles[k];
		mincycles = MIN(mincycles, cycles[k]);
		maxcycles = MAX(maxcycles, cycles[k]);

		ratio = maxdiff[k]/ (maxtimeTh[k]);
		maxratio = MAX(maxratio, ratio);
		diffmax = MAX(diffmax, maxdiff[k]);
	}
	avgtime = avgtime/(double)(NTIMES-1);
	avgcycles = avgcycles/(double)(NTIMES-1);

	double ldps = (double)bytes[0] * K / mintime;
	printf("Function      Rate (B/s)   Avg time     Min time     Max time     Gfreq\n");
	printf("%s%11.4f%c  %11.6f  %11.6f  %11.6f  %11.6f\n", "Load   ",
			 siz_d(ldps),
			 units_d(ldps),
			avgtime,
			mintime,
			maxtime,
			1.0E-09 * avgcycles/avgtime);
	printf("Time diff max between threads= %f\n", diffmax);
	printf("Maximum ratio diff time/ max time= %f%% \n", maxratio * 100);
	
	free(oldarr);
	return 0;
}





void load(double const *restrict t, int n, intptr_t k) 
{
	trash=VSETZERO(); 
	assert(n%4 == 0);
	register __m256d res={0,0,0,0};
	register __m256d res2={0,0,0,0};
	register __m256d res3={0,0,0,0};
	register __m256d res4={0,0,0,0};
	register __m256d res5={0,0,0,0};
	register __m256d res6={0,0,0,0};
	register __m256d res7={0,0,0,0};
	register __m256d res8={0,0,0,0};
	//register __m256d resm={1,2,1,3};
	//register __m256d resm={1,2,1,3};
	//register __m256d resm={1,2,1,3};
	//register __m256d resm={1,2,1,3};
	register __m256d ldvec, ldvec2, ldvec3, ldvec4, ldvec5, ldvec6, ldvec7, ldvec8;
	for (intptr_t j = 0; j < k; j++)
	{
		for (intptr_t i = 0; i < n; i +=32)
		{
			ldvec = _mm256_load_pd(t + i);
			res = _mm256_add_pd(res, ldvec);
			ldvec2 = _mm256_load_pd(t + i + 4);
			res2 = _mm256_add_pd(res, ldvec2 );
			ldvec3 = _mm256_load_pd(t + i + 8);
			res3 = _mm256_add_pd(res3, ldvec3);
			ldvec4 = _mm256_load_pd(t + i + 12);
			res4 = _mm256_add_pd(res4, ldvec4);
			ldvec5 = _mm256_load_pd(t + i + 16);
			res5 = _mm256_add_pd(res5, ldvec5);
			ldvec6 = _mm256_load_pd(t + i + 20);
			res6 = _mm256_add_pd(res6, ldvec6);
			ldvec7 = _mm256_load_pd(t + i + 24);
			res7 = _mm256_add_pd(res7, ldvec7);
			ldvec8 = _mm256_load_pd(t + i + 28);
			res8 = _mm256_add_pd(res8, ldvec8);
			//ldvecm = _mm256_load_pd(t + i + 4);
			//resm = _mm256_mul_pd(resm, ldvecm);
		}
	}

	trash = _mm256_add_pd(trash, res);
	trash = _mm256_add_pd(trash, res2);
	trash = _mm256_add_pd(trash, res3);
	trash = _mm256_add_pd(trash, res4);
	trash = _mm256_add_pd(trash, res5);
	trash = _mm256_add_pd(trash, res6);
	trash = _mm256_add_pd(trash, res7);
	trash = _mm256_add_pd(trash, res8);
}

void load_asm(double const * t, intptr_t n, intptr_t k) 
{
	//prevent dead code optimization
	trash=_mm256_setzero_pd(); 
	assert(n%4 == 0);
	__m256d ldvec= {0,0,0,0};
	intptr_t nbvloads= 0;
	asm volatile (
			//initialize load counter
			//"movq $0, %%rcx;"
			//"movq %%rcx, %1;"


			//initialize outer loop counter
			"movq $0, %%rbx;"
			"cmpq %4, %%rbx;"
			"jge 3f;"
			
			"4:;"

			//initialize loop counter
			"movq $0, %%rax;"
			"cmpq %3, %%rax;"
			"jge 2f;"



			"1:;"
			//vload from memory
			"vmovapd (%2, %%rax, 8), %%ymm1;"
			//increment vload counter
			//"addq $1, %%rcx;"
			//increment and compare
			"addq $4, %%rax;"
			"cmpq %3, %%rax;"
			"jl 1b;"



			"2:;"
			"vmovapd %%ymm1, %0;"

			//outer loop test
			"addq $1, %%rbx;"
			"cmpq %4, %%rbx;"
			"jl 4b;"

			"3:;"
			//"movq %%rcx, %1;"


			: "=m" (ldvec) , "=m" ( nbvloads )
			:"r" ( t ), "r" ( n ), "r" ( k )
			:"%ymm1", "%rax", "%rbx"//, "%rcx"
		     );
	//printf("nb loads done= %ld, expected %ld \n", nbvloads, n * k / 4 );
}

void load_asm_b(double const * t, int n)
{
	trash=VSETZERO(); 
	assert(n%4 == 0);
	__m256d res={0,0,0,0};
	for (intptr_t i = 0; i < n; i +=4)
	{
		__m256d ldvec;

		asm volatile (
				"vmovapd (%1, %2, 8), %%ymm1;"
				"vmovapd %%ymm1, %0;"
				: "=m" (ldvec)
				:"r" ( t ), "r" ( i )
				:"%ymm1"
		   );
		res = _mm256_add_pd(res, ldvec);
	}
	trash = _mm256_add_pd(trash, res);
}
# define	M	20


double mysecond2()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp, &tzp);
	if ( i < 0) 
	{
		printf("error getting time\n");
		exit(EXIT_FAILURE);
       	}
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
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

//get number of cycles  since last cpu reset
static inline unsigned long get_cycles()
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


//returns the pointer passed as argument round up to n
//n must be a power of 2
double * align_ptr(double * pt, int n)
{
	intptr_t ic = (intptr_t) pt;
	ic = (ic + n) & ~(n-1);
	return (double *)ic;
}


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
