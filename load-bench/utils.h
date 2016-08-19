#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <immintrin.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <regex>

#define M 20
# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif


# ifndef DOUBLE_PER_CYCLE
#define DOUBLE_PER_CYCLE 2
#endif

#ifndef TOTAL_CYCLES
#define TOTAL_CYCLES ((long)1 << 32)
#endif
char units(unsigned int n);
int siz(unsigned int n);
char units_d(double f);
double siz_d(double n);
inline unsigned long get_cycles();
int checktick();
double mysecond();
void print_status();
long get_nb_iter(long size);
long *  get_tab_iter(int * sizes, int nsiz);
template <typename T> T array_sum(T* arr, int siz);
