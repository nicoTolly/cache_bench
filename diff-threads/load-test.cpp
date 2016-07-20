#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <immintrin.h>
#include "parser.h"
#include "utils.h"


#define K (1<<8)
void * handler(void * arg);
void * handler_slw(void * arg);
char * align_ptr(char * t, intptr_t n);
void __attribute__((optimize("O0"))) load_asm(double const * t, intptr_t n, intptr_t k) ;
void __attribute__((optimize("O0"))) load_asm_slw(double const * t, intptr_t n, intptr_t k) ;

__m256d trash;

ThrParam *param;


//arguments passed
//to thread handler
typedef struct 
{
	double *t;
	pthread_barrier_t * bar;
	long size;
	long niter;
} args_t;





int main(int argc, char ** argv)
{
	double * tab;
	pthread_t * thrTab;
	pthread_barrier_t bar;


	int quantum;
	int BytesPerWord;


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
	
	


	int nbFst = param->nbThread - param->nbSlow;
	//initializing tab containing data to be loaded
	tab = new double[param->globsiz + 8];
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
	for (long i = 0; i <  param->globsiz; i++)
	{
		tab[i]= 3.4;
	}


	printf(HLINE);
	unsigned int bytes= param->globsiz * sizeof(double);
	printf("N = %ld, %d threads will be called, loading %d%c bytes of data %d times\n", param->globsiz, param->nbThread, siz(bytes), units(bytes), K);
	printf(HLINE);



	// array that will contain pthread identifiers
	thrTab = (pthread_t *)malloc(param->nbThread * sizeof(pthread_t));
	
	*thrTab= pthread_self();
	pthread_barrier_init(&bar, NULL, param->nbThread);
	args_t hargs[param->nbThread];


	if (param->thrSizes == NULL)
	{


		int i;
		intptr_t offset = 0;
		if (param->nbSlow == param->nbThread)
			offset += param->ssiz;
		else
			offset += param->fsiz;

		for(i = 1; i < nbFst; i++)
		{
			hargs[i].t = tab + offset;
			hargs[i].bar = &bar; 
			hargs[i].size = param->fsiz; 
			hargs[i].niter = K; 
			pthread_create(thrTab + (intptr_t)i, NULL, handler, (void *) &hargs[i]);
			offset +=  param->fsiz;
		}

		for(; i < param->nbThread; i++)
		{
			hargs[i].t = tab + offset;
			hargs[i].bar = &bar; 
			hargs[i].size = param->ssiz; 
			hargs[i].niter = K; 
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
			hargs[i].niter = K; 
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
	hargs[0].niter = K; 
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
	else
	{
		hargs[0].size = param->thrSizes[0];
		handler_slw((void *)&hargs[0]);
	}


	for (int k = 0; k < param->nbThread; k++)
		pthread_join(thrTab[k], NULL);
	/*
	for (int k = 0; k < nbFst; k++)
		pthread_join(thrTab[k], NULL);
		*/
	


	//free param
	delete param;
	free(thrTab);
	free(oldtab);
	return 0;
}



void * handler(void * arg)
{
	args_t * args = (args_t *) arg;
	double ld = sizeof(double) * args->size * args->niter;
	double time;


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);


	time = mysecond();
	load_asm(args->t, args->size, args->niter);
	time = mysecond() - time;
	printf("Fast thread has taken %11.8f s to execute\n \
Throughput :%f %cB/s \n", time, siz_d(ld / time), units_d(ld / time));

	return NULL;
}

void * handler_slw(void * arg)
{
	args_t * args = (args_t *) arg;
	double ld = sizeof(double) * args->size * args->niter;
	double time;


	// barrier (waiting for everyone to be pinned)
	pthread_barrier_wait(args->bar);


	time = mysecond();
	//load_asm_slw(args->t, args->size, args->niter);
	load_asm(args->t, args->size, args->niter);
	time = mysecond() - time;
	printf("Slow thread has taken %11.8f s to execute\n \
Throughput :%f %cB/s \n", time, siz_d(ld / time), units_d(ld / time));
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

/*
void load_asm(double * t, int n, int k)
{
	asm(
			
//"			movq %0, %%rsi;"
"			movq %1, %%rbx;"
"			movq %2, %%rdx;"

"			movq $0, %%rcx;"
"			cmpq %%rdx, %%rcx;"
"			jge 4f;"


"			1:;"
"			movq $0, %%rcx;"
"			cmpq %%rdx, %%rcx;"
"			jge 3f;"

"			2:;"
"			addq $1, %%rax;"
"			cmpq %%rbx, %%rax;"
"			jl 1b;"


"			3:;"

"			addq $1, %%rcx;"
"			cmpq %%rcx, %%rdx;"
"			jl 1b;"
			
"			4:;"
			
	:	
	: "g"(t),  "g"(n),  "g"(k)
	: "%rsi","%rax", "%rbx", "%rcx", "%rdx"
	); 
}
*/

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



void load_asm_slw(double const * t, intptr_t n, intptr_t k) 
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

