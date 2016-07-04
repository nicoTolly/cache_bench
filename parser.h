#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

#ifndef THR_PARAM
#define THR_PARAM
	
#define NCORES_PER_NODE 4
// implementation in parser.cpp
class ThrParam
{
	public:

	//every fields
	
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
};

#endif // THR_PARAM

int parseArg (int argc, char * args[], ThrParam ** param);
