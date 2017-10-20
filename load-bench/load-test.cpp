#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <immintrin.h>
#include "parser.h"
#include "utils.h"
#include <sched.h>
#include <stdint.h>

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

#define K (1<<12)
void * handler(void * arg);
void * handler_slw(void * arg);
char * align_ptr(char * t, intptr_t n);


extern "C" int ld_vect(double * t, uint64_t n, uint64_t k);
extern "C" int ld_vect_slw(double * t, uint64_t n, uint64_t k);
extern "C" unsigned long get_cycles();

#undef USE_PADDING
void __attribute__((optimize("O0"))) load_asm_sse(double const * t, intptr_t n, intptr_t k) ;


//inline unsigned long get_cycles();

__m256d trash;

ThrParam *param;


//arguments passed
//to thread handler
typedef struct 
{
	double *t;
	double * time;
	pthread_barrier_t * bar;
	uint64_t size;
	uint64_t niter;
	unsigned long *cycle;
} args_t;





int main(int argc, char ** argv)
{
	double * tab;
	pthread_t * thrTab;
	pthread_barrier_t bar;


	int quantum;
	int BytesPerWord;
	uint64_t nb_iter;
	unsigned long load_bytes;
#ifdef USE_PADDING
// We introduce some padding in data to see if 
// something is changed
	int var_padding = (1 << 25);
#endif


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
	//storing old value for freeing
	double * oldtab = tab;
	//aligning on 32 bytes for intrinsics and
	//assembly constraint
	tab = (double *)align_ptr((char *)tab, 32);

#else
#ifdef USE_PADDING
	//padding
	//tab = new double[param->globsiz + 8 + param->nbThread * var_padding];
	int r = posix_memalign((void **) &tab, 32, param->globsiz * sizeof(double) + param->nbThread * var_padding * sizeof(double));
	if (r < 0)
	{
	  printf("memalign failure\n");
	  return -1;
	}
#else//USE_PADDING
	//tab = new double[param->globsiz + 8 ];
	int r = posix_memalign((void **) &tab, 32, param->globsiz * sizeof(double));
	if (r != 0)
	{
	  printf("memalign failure\n");
	  return -1;
	}
#endif//USE_PADDING
#endif //HUGE
#pragma omp for
	for (size_t i = 0; i <  param->globsiz; i++)
	{
		tab[i]= 3.4;
	}

	printf(HLINE);
	print_status();
	printf(HLINE);
	

	// getting number of iterations, we want to do approximately 2^25 cycles

	long * iter = NULL;
	load_bytes = 0;
	if (param->thrSizes != NULL)
	{
		iter = get_tab_iter(param->thrSizes, param->nbThread);
		nb_iter = 0;
		for (int i = 0; i < param->nbThread; i++)
		{
			//nb_iter += iter[i] ;
			load_bytes += iter[i] * param->thrSizes[i] * sizeof(double); 
		}
		nb_iter = array_sum(iter, param->nbThread);
}
	else
	{
		nb_iter = get_nb_iter(param->globsiz);
		load_bytes = nb_iter * param->globsiz;
	}


	long bytes = param->globsiz * sizeof(double);
	printf(HLINE);
	printf("N = %ld, %d threads will be called, loading %d%c bytes of data \n", param->globsiz, param->nbThread, siz(bytes), units(bytes));
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
#ifdef USE_PADDING
			hargs[i].t = tab + offset + var_padding;
#else//USE_PADDING
			hargs[i].t = tab + offset ;
#endif//USE_PADDING
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
		int i;
		for( i = 1; i < nbFst; i++)
		{
			//padding with adding_var doubles
			hargs[i].t = tab + offset ;
			hargs[i].bar = &bar; 
			hargs[i].size = param->thrSizes[i]; 
			//hargs[i].niter = nb_iter; 
			hargs[i].niter = iter[i]; 
			hargs[i].time = times + i; 
			hargs[i].cycle = cycles + i; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler, (void *) &hargs[i]);
			offset +=  param->thrSizes[i];
		}
		for(; i < param->nbThread; i++)
		{
			//padding with adding_var doubles
			hargs[i].t = tab + offset ;
			hargs[i].bar = &bar; 
			hargs[i].size = param->thrSizes[i]; 
			//hargs[i].niter = nb_iter; 
			hargs[i].niter = iter[i]; 
			hargs[i].time = times + i; 
			hargs[i].cycle = cycles + i; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler_slw, (void *) &hargs[i]);
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
			s = pthread_setaffinity_np(thrTab[k],sizeof(cpu_set_t), &sets[k]);
			if (s != 0)
			{
				printf("could not set affinity\n");
				exit(EXIT_FAILURE);
			}
			printf("fast thread [%d] pinned on cpu %d\n", k, param->fstThrList[k]);
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
	//hargs[0].niter = nb_iter; 
	if (iter != NULL)
		hargs[0].niter = iter[0]; 
	else
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
		if(param->nbThread == param->nbSlow)
			handler_slw((void *)&hargs[0]);
		else
			handler((void *)&hargs[0]);
			

	}


	for (int k = 0; k < param->nbThread; k++)
		pthread_join(thrTab[k], NULL);
	
	double maxtime = *(std::max_element(times, times + param->nbThread));
	unsigned long maxcycle = *(std::max_element(cycles, cycles + param->nbThread));

	//double ld = sizeof(double) * param->globsiz * nb_iter ;
	printf("Global throughput : %f %cB/s\n", siz_d(load_bytes / maxtime), units_d(load_bytes / maxtime));
	printf("Global bytes per cycle : %f %cB/c\n", siz_d(load_bytes / maxcycle), units_d(load_bytes / maxcycle));
	printf("Estimated frequency : %f %cHz\n", siz_d(maxcycle / maxtime), units_d(maxcycle / maxtime));

	//free param
	delete[] times;
	delete[] cycles;
	free(thrTab);
#if defined(__gnu_linux__) && defined(USE_HUGE) 
	munmap((void *)oldtab, ((param->globsiz + 8) * sizeof(double) ));
	//munmap((void *)oldtab, std::min((size_t)(1 << 27), ((param->globsiz + 8) * sizeof(double))));
#else
	free(tab);
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
	int cpu;


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);

	cpu =  sched_getcpu();


	cyc = get_cycles();
	time = mysecond();
	//load_asm(args->t, args->size, args->niter);
	ld_vect(args->t, args->size, args->niter);
	time = mysecond() - time;
	cyc = get_cycles() - cyc;
	*(args->time) = time;
	*(args->cycle) = cyc;
	printf("Fast thread has taken %11.8f s to execute on cpu %d, data : %ld bytes loaded %ld times\n \
Throughput : %f %cB/s \n", time, cpu, args->size * sizeof(double),  args->niter, siz_d(ld / time), units_d(ld / time));

	return NULL;
}

void * handler_slw(void * arg)
{
	args_t * args = (args_t *) arg;
	double ld = sizeof(double) * args->size * args->niter;
	double time;
	unsigned long cyc; 
	int cpu;


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);

	cpu =  sched_getcpu();


	cyc = get_cycles();
	time = mysecond();
	ld_vect_slw(args->t, args->size, args->niter);
	time = mysecond() - time;
	cyc = get_cycles() - cyc;
	*(args->time) = time;
	*(args->cycle) = cyc;
	printf("Slow thread has taken %11.8f s to execute on cpu %d, data : %ld bytes loaded %ld times\n \
Throughput : %f %cB/s \n", time, cpu, args->size * sizeof(double),  args->niter, siz_d(ld / time), units_d(ld / time));
	return NULL;
}

char * align_ptr(char * t, intptr_t n)
{
	intptr_t ptr = (intptr_t) t;

	ptr = ((ptr % n == 0) ? ptr :(ptr + n) & ~(n - 1));
	return (char *)ptr;
}
