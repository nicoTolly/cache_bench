#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <hwloc.h>

using namespace std;

#ifndef THR_PARAM
#define THR_PARAM
	
#define NCORES_PER_NODE 8
int getCpuPerNode();


// implementation in parser.cpp
class ThrParam
{
	public:

	//every fields
	
	
	//standard size of array
	long size;
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

	//constructor
	ThrParam();
	//ThrParam(int nbThread);
	ThrParam(int nbThread, int nbSlow);
	ThrParam(int nbThread, int nbSlow, bool close);
	//destructor
	~ThrParam();

	//print every info about parameters
	void info();

	private:

	void init();
};

#endif // THR_PARAM

int parseArg (int argc, char * args[], ThrParam ** param);
