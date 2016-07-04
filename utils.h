#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <immintrin.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#define M 20
# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

char units(unsigned int n);
int siz(unsigned int n);
inline unsigned long get_cycles();
int checktick();
double mysecond();
