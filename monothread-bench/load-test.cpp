#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <immintrin.h>
#include "utils.h"

#include <algorithm>


//#ifdef __gnu_linux__
#if defined(__gnu_linux__) && defined(USE_HUGE) 
#include <sys/mman.h>
// these macros should have been defined
// but strangely, they are not
#define HUGE_MAP_1GB (30 << MAP_HUGE_SHIFT)
#define HUGE_MAP_2MB (21 << MAP_HUGE_SHIFT)
#define HUGE_MAP HUGE_MAP_1GB
#endif

#define NTAB (1<<23)
#define K (1<<10)
void * handler(void * arg);
void * handler_slw(void * arg);
char * align_ptr(char * t, intptr_t n);
void __attribute__((optimize("O0"))) load_asm(double const * t, intptr_t n, intptr_t k) ;
void __attribute__((optimize("O0"))) load_asm2(double const * t, intptr_t n, intptr_t k) ;
void __attribute__((optimize("O0"))) load_asm_slw(double const * t, intptr_t n, intptr_t k) ;
inline unsigned long get_cycles();

__m256d trash;



//arguments passed
//to thread handler
typedef struct 
{
	double *t;
	double * time;
	long size;
	long niter;
	unsigned long *cycle;
} args_t;


using namespace std;


int main(int argc, char ** argv)
{
	double * tab;



	int quantum;
	int BytesPerWord;


	//Parsing args and initializing param
	//in order to get number of threads,
	//slow threads, places, etc.
	/*
	int ret = parseArg(argc, argv,  &param);
	if (ret < 0)
	{
		printf("usage : ./load -n=<threads number> -slow=<number of slow threads> -size=<power of two> -ratio=<ratio slow size / fast size> -places=<core places| close | spread>\n");
		exit(EXIT_FAILURE);
	}
	if (param == NULL)
	{
		cout << "param not allocated !!" << endl;
		exit(EXIT_FAILURE);
	}

	param->info();
	*/
	


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
	
	cpu_set_t sets;
	CPU_ZERO(&sets);
	CPU_SET(0, &sets);
	int s = pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t), &sets);
	if (s != 0)
	{
		printf("could not set affinity\n");
		exit(EXIT_FAILURE);
	}
	


	//initializing tab containing data to be loaded
#if defined(__gnu_linux__) && defined(USE_HUGE) 
	//using mmap for allocating hugepages
	void * ptrvoid = mmap(NULL, (param->globsiz + 8) * sizeof(double), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | HUGE_MAP , -1, 0 );
	if (ptrvoid == MAP_FAILED)
	{
		cout << "could not allocate tab with mmap" << endl;
		exit(EXIT_FAILURE);
	}
	tab = (double *) ptrvoid;

#else
	tab = new double[NTAB + 8];
#endif
	if(tab == NULL)
	{
		cout << "could not allocate tab" << endl;
		exit(EXIT_FAILURE);
	}
	//storing old value for freeing
	double * oldtab = tab;
	//aligning on 32 bytes for intrinsics and
	//assembly constraint
	tab = (double *)align_ptr((char *)tab, 32);
#pragma omp for
	for (size_t i = 0; i <  NTAB; i++)
	{
		tab[i]= 3.4;
	}


	printf(HLINE);
	unsigned int bytes = NTAB * sizeof(double);
	printf("N = %ld, 1 threads will be called, loading %d%c bytes of data %d times\n", NTAB, siz(bytes), units(bytes), K);
	printf(HLINE);


	args_t hargs;

	double time;
	unsigned long cycle;

	hargs.t = tab ;
	hargs.niter = K; 
	hargs.time = &time ; 
	hargs.cycle = &cycle; 
	hargs.size = NTAB ;
	handler((void *)&hargs);
	//case user had entered thread sizes as inputs


	

	double ld = sizeof(double) * NTAB * K ;
	printf("Global throughput : %f %cB/s\n", siz_d(ld / time), units_d(ld / time));
	printf("Global bytes per cycle : %f %cB/c\n", siz_d(ld / cycle), units_d(ld / cycle));
	printf("Estimated frequence : %f %cHz\n", siz_d(cycle / time), units_d(cycle / time));

	//free param
#if defined(__gnu_linux__) && defined(USE_HUGE) 
	munmap((void *)oldtab, ((param->globsiz + 8) * sizeof(double) ));
	//munmap((void *)oldtab, std::min((size_t)(1 << 27), ((param->globsiz + 8) * sizeof(double))));
#else
	free(oldtab);
#endif
	return 0;
}



void * handler(void * arg)
{
	args_t * args = (args_t *) arg;
	//size of data that are to be loaded 
	//by this thread
	double ld = sizeof(double) * args->size * args->niter;
	double time;
	unsigned long cyc; 

	/*

	cpu_set_t sets;
	CPU_ZERO(&sets);
	CPU_SET(0, &sets);
	int s = pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t), &sets);
	if (s != 0)
	{
		printf("could not set affinity\n");
		exit(EXIT_FAILURE);
	}

	*/

	cyc = get_cycles();
	time = mysecond();
	//load_asm(args->t, args->size, args->niter);
	load_asm2(args->t, args->size, args->niter);
	time = mysecond() - time;
	cyc = get_cycles() - cyc;
	*(args->time) = time;
	*(args->cycle) = cyc;
	printf("Fast thread has taken %11.8f s to execute, data : %ld bytes\n \
Throughput : %f %cB/s \n", time, args->size * sizeof(double),  siz_d(ld / time), units_d(ld / time));

	return NULL;
}

void * handler_slw(void * arg)
{
	args_t * args = (args_t *) arg;
	double ld = sizeof(double) * args->size * args->niter;
	double time;
	unsigned long cyc; 




	cyc = get_cycles();
	time = mysecond();
	load_asm_slw(args->t, args->size, args->niter);
	//load_asm(args->t, args->size, args->niter);
	time = mysecond() - time;
	cyc = get_cycles() - cyc;
	*(args->time) = time;
	*(args->cycle) = cyc;
	printf("Slow thread has taken %11.8f s to execute, data : %ld bytes\n \
Throughput :%f %cB/s \n", time, args->size * sizeof(double),  siz_d(ld / time), units_d(ld / time));
	return NULL;
}

//takes a pointer and returns it
//rounded up on upper n bound
char * align_ptr(char * t, intptr_t n)
{
	intptr_t ptr;
	ptr = (intptr_t)(t + n) & ~(n - 1);
	return (char *)ptr;
}


void load_asm(double const * t, intptr_t n, intptr_t k) 
{
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
			//"vmovapd %%ymm1, %0;"

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

void load_asm2(double const * t, intptr_t n, intptr_t k) 
{
	assert(n%32 == 0);
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
			"vmovapd (%2, %%rax, 8), %%ymm0;"
			"vmovapd 32(%2, %%rax, 8), %%ymm1;"
			"vmovapd 64(%2, %%rax, 8), %%ymm2;"
			"vmovapd 96(%2, %%rax, 8), %%ymm3;"
			"vmovapd 128(%2, %%rax, 8), %%ymm4;"
			"vmovapd 160(%2, %%rax, 8), %%ymm5;"
			"vmovapd 192(%2, %%rax, 8), %%ymm6;"
			"vmovapd 224(%2, %%rax, 8), %%ymm7;"
			//increment vload counter
			//"addq $1, %%rcx;"
			//increment and compare
			"addq $32, %%rax;"
			"cmpq %3, %%rax;"
			"jl 1b;"



			"2:;"
			//"vmovapd %%ymm1, %0;"

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


void load_asm_slw(double const * t, intptr_t n, intptr_t k) 
{
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
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"nop;"
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
			"jle 4b;"

			"3:;"
			//"movq %%rcx, %1;"


			: "=m" (ldvec) , "=m" ( nbvloads )
			:"r" ( t ), "r" ( n ), "r" ( k )
			:"%ymm1", "%rax", "%rbx"//, "%rcx"
		     );
	//printf("nb loads done= %ld, expected %ld \n", nbvloads, n * k / 4 );
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
