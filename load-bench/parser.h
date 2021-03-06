
#ifndef _PARS_
#define _PARS_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <list>
#ifdef HWLOC
#include <hwloc.h>
#endif // HWLOC

using namespace std;
	


#define NDEFAULT (1 << 20)
#define NCPU_PER_SOCK 4
int getCpuPerNode();

int getSizes(int ** thr, int * nbT);

// implementation in parser.cpp
class ThrParam
{
	public:

	//every fields
	
	
	size_t globsiz;
	//standard size of array
	uint64_t fsiz;
	//standard size of array
	uint64_t ssiz;
	// number of CPU per NUMA node
	int nbPUNode;
	//total number of threads
	int nbThread; 
	// number of slow threads
	int nbSlow;
	// do we put threads on same node or on two nodes ?
	// default is true (on same node)
	bool close;
	int *  fstThrList;
	int *  slwThrList;

	int *  thrSizes;
	//constructor
	ThrParam();
	//ThrParam(int nbThread);
	ThrParam(int nbThread, int nbSlow);
	ThrParam(int nbThread, int nbSlow, bool close);
	ThrParam(int nbThread, int nbSlow, bool close, int siz);
	ThrParam(int nbThread, int nbSlow, bool close, int siz, float ratio);
	ThrParam(int nbThread, int** thrSize, int nbSlw, bool close);
	//destructor
	~ThrParam();

	//print every info about parameters
	void info();

	private:

	void init();
};


int parseArg (int argc, char * args[], ThrParam ** param);

#endif // _PARS_
