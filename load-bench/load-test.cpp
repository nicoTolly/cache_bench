#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <immintrin.h>
#include "parser.h"
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

#define K (1<<10)
void * handler(void * arg);
void * handler_slw(void * arg);
char * align_ptr(char * t, intptr_t n);
#ifdef USE_AVX
void __attribute__((optimize("O0"))) load_asm(double const * t, intptr_t n, intptr_t k) ;
void __attribute__((optimize("O0"))) load_asm2(double const * t, intptr_t n, intptr_t k) ;
void __attribute__((optimize("O0"))) load_asm_slw(double const * t, intptr_t n, intptr_t k) ;
#endif

void __attribute__((optimize("O0"))) load_asm_sse(double const * t, intptr_t n, intptr_t k) ;


inline unsigned long get_cycles();

__m256d trash;

ThrParam *param;


//arguments passed
//to thread handler
typedef struct 
{
	double *t;
	double * time;
	pthread_barrier_t * bar;
	long size;
	long niter;
	unsigned long *cycle;
} args_t;





int main(int argc, char ** argv)
{
	double * tab;
	pthread_t * thrTab;
	pthread_barrier_t bar;


	int quantum;
	int BytesPerWord;
	long nb_iter;


	//Parsing args and initializing param
	//in order to get number of threads,
	//slow threads, places, etc.
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
	
	
	//pinning main thread on cpu 0 (on a numa architecture, data could 
	// be allocated on another node otherwise)
	cpu_set_t sets;
	CPU_ZERO(&sets);
	CPU_SET(0, &sets);
	int s = pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t), &sets);
	if (s != 0)
	{
		printf("could not set affinity\n");
		exit(EXIT_FAILURE);
	}
	
	


	int nbFst = param->nbThread - param->nbSlow;
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
	tab = new double[param->globsiz + 8];
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
	for (size_t i = 0; i <  param->globsiz; i++)
	{
		tab[i]= 3.4;
	}

	printf(HLINE);
	print_status();
	printf(HLINE);
	

	// getting number of iterations, we want to do approximately 2^25 cycles
	nb_iter = get_nb_iter(param->globsiz);

	printf(HLINE);
	unsigned int bytes = param->globsiz * sizeof(double);
	printf("N = %ld, %d threads will be called, loading %d%c bytes of data %ld times\n", param->globsiz, param->nbThread, siz(bytes), units(bytes), nb_iter);
	printf(HLINE);



	// array that will contain pthread identifiers
	thrTab = (pthread_t *)malloc(param->nbThread * sizeof(pthread_t));
	
	*thrTab= pthread_self();
	// we don't want threads to start loading before having been
	// pinned, so we put a barrier
	pthread_barrier_init(&bar, NULL, param->nbThread);
	args_t hargs[param->nbThread];

	//array for timestamp (in order to get global throughput
	// at the end)
	double *times = new double[param->nbThread];
	unsigned long *cycles = new unsigned long[param->nbThread];
	


	if (param->thrSizes == NULL)
	{


		int i;
		// this offset will be incremented by the size
		// of the array we want our thread to work on
		intptr_t offset = 0;
		if (param->nbSlow == param->nbThread)
			offset += param->ssiz;
		else
			offset += param->fsiz;

		for(i = 1; i < nbFst; i++)
		{
			//address of data start for this thread
			hargs[i].t = tab + offset;
			hargs[i].bar = &bar; 
			hargs[i].size = param->fsiz; 
			hargs[i].niter = nb_iter; 
			hargs[i].time = times + i; 
			hargs[i].cycle = cycles + i; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler, (void *) &hargs[i]);
			offset +=  param->fsiz;
		}

		for(; i < param->nbThread; i++)
		{
			hargs[i].t = tab + offset;
			hargs[i].bar = &bar; 
			hargs[i].size = param->ssiz; 
			hargs[i].niter = nb_iter; 
			hargs[i].time = times + i; 
			hargs[i].cycle = cycles + i; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler_slw, (void *) &hargs[i]);
			offset +=  param->ssiz;
		}

	}
	// thrSizes is set
	// user has entered data sizes
	else
	{
		intptr_t offset = param->thrSizes[0];
		for(int i = 1; i < param->nbThread; i++)
		{
			hargs[i].t = tab + offset;
			hargs[i].bar = &bar; 
			hargs[i].size = param->thrSizes[i]; 
			hargs[i].niter = nb_iter; 
			hargs[i].time = times + i; 
			hargs[i].cycle = cycles + i; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler, (void *) &hargs[i]);
			offset +=  param->thrSizes[i];
		}
	}



	//Placing threads on appropriate cpu
	//If either fstThrList or slwThrList
	//is set to null, then it means that 
	//all threads are either fast or slow
	if (param->fstThrList != NULL)
	{
		int s;
		cpu_set_t sets[nbFst];
		for (int k = 0; k < nbFst; k++)
		{
			CPU_ZERO(&sets[k]);
			CPU_SET(param->fstThrList[k], &sets[k]);
			printf("fast thread [%d] pinned on cpu %d\n", k, param->fstThrList[k]);
			s = pthread_setaffinity_np(thrTab[k],sizeof(cpu_set_t), &sets[k]);
			if (s != 0)
			{
				printf("could not set affinity\n");
				exit(EXIT_FAILURE);
			}
		}
		
	}

	if (param->slwThrList != NULL)
	{
		int s;
		cpu_set_t sets[param->nbSlow];
		for (int k = 0; k < param->nbSlow; k++)
		{
			CPU_ZERO(&sets[k]);
			CPU_SET(param->slwThrList[k], &sets[k]);
			s = pthread_setaffinity_np(thrTab[nbFst + k],sizeof(cpu_set_t), &sets[k]);
			if (s != 0)
			{
				printf("could not set affinity\n");
				exit(EXIT_FAILURE);
			}
			printf("Slow thread [%d] pinned on cpu %d\n", k, param->slwThrList[k]);
		}

	}


	// launching main thread
	// slow threads if there is 
	// no fast thread
	// fast thread otherwise
	hargs[0].t = tab ;
	hargs[0].bar = &bar; 
	hargs[0].niter = nb_iter; 
	hargs[0].time = times ; 
	hargs[0].cycle = cycles; 
	if (param->thrSizes == NULL)
	{
		if(param->nbThread == param->nbSlow)
		{
			hargs[0].size = param->ssiz; 
			handler_slw((void *)&hargs[0]);
		}
		else
		{
			hargs[0].size = param->fsiz; 
			handler((void *)&hargs[0]);
		}
	}
	//case user had entered thread sizes as inputs
	else
	{
		hargs[0].size = param->thrSizes[0];
		handler((void *)&hargs[0]);
	}


	for (int k = 0; k < param->nbThread; k++)
		pthread_join(thrTab[k], NULL);
	
	double maxtime = *(std::max_element(times, times + param->nbThread));
	unsigned long maxcycle = *(std::max_element(cycles, cycles + param->nbThread));

	double ld = sizeof(double) * param->globsiz * nb_iter ;
	printf("Global throughput : %f %cB/s\n", siz_d(ld / maxtime), units_d(ld / maxtime));
	printf("Global bytes per cycle : %f %cB/c\n", siz_d(ld / maxcycle), units_d(ld / maxcycle));
	printf("Estimated frequency : %f %cHz\n", siz_d(maxcycle / maxtime), units_d(maxcycle / maxtime));

	//free param
	delete[] times;
	delete[] cycles;
	free(thrTab);
#if defined(__gnu_linux__) && defined(USE_HUGE) 
	munmap((void *)oldtab, ((param->globsiz + 8) * sizeof(double) ));
	//munmap((void *)oldtab, std::min((size_t)(1 << 27), ((param->globsiz + 8) * sizeof(double))));
#else
	free(oldtab);
#endif
	delete param;
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


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);


	cyc = get_cycles();
	time = mysecond();
	//load_asm(args->t, args->size, args->niter);
#ifdef USE_AVX
	load_asm2(args->t, args->size, args->niter);
#else
	load_asm_sse(args->t, args->size, args->niter);
#endif
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


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);


	cyc = get_cycles();
	time = mysecond();
#ifdef USE_AVX
	load_asm_slw(args->t, args->size, args->niter);
#else
#endif
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

#ifdef USE_AVX

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


#endif

void load_asm_sse(double const * t, intptr_t n, intptr_t k) 
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
			"vmovapd (%2, %%rax, 8), %%xmm0;"
			"vmovapd 16(%2, %%rax, 8), %%xmm4;"
			"vmovapd 32(%2, %%rax, 8), %%xmm1;"
			"vmovapd 48(%2, %%rax, 8), %%xmm5;"
			"vmovapd 64(%2, %%rax, 8), %%xmm2;"
			"vmovapd 80(%2, %%rax, 8), %%xmm6;"
			"vmovapd 96(%2, %%rax, 8), %%xmm3;"
			"vmovapd 112(%2, %%rax, 8), %%xmm7;"
			//increment vload counter
			//"addq $1, %%rcx;"
			//increment and compare
			"addq $16, %%rax;"
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
			:"%xmm1", "%rax", "%rbx"//, "%rcx"
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
