#include "parser.h"

using namespace std;


ThrParam::ThrParam():nbSlow(0),close(true), slwThrList(NULL)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCORES_PER_NODE;
	else
		nbPUNode = res;
	nbThread = nbPUNode;
	this->init();
}

ThrParam::ThrParam(int nbT, int nbSlw):nbSlow(nbSlw),close(true)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCORES_PER_NODE;
	else
		nbPUNode = res;

	// setting default nb thread if 
	// it is not passed in parameter
	if (nbT == -1)
		nbThread = nbPUNode;
	else
		nbThread = nbT;
	this->init();

}

ThrParam::ThrParam(int nbT, int nbSlw, bool close):nbSlow(nbSlw), close(close)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCORES_PER_NODE;
	else
		nbPUNode = res;
	if (nbT == -1)
		nbThread = nbPUNode;
	else
		nbThread = nbT;
	this->init();
}

void ThrParam::init()
{
	if (nbSlow == nbThread)
	{
		this->fstThrList = NULL;
		this->slwThrList = new int[nbSlow];
		for(int i = 0; i < nbThread; i++)
			// take modulo NCORES_PER_NODE cause we have NCORES_PER_NODE cores
			this->slwThrList[i] = i % nbPUNode;
	}
	else if(close)
		//case we want to place every
		//thread on a node (same cache)
	{
		int nbFst = nbThread - nbSlow;
		this->fstThrList = new int[nbFst];
		this->slwThrList = new int[nbSlow];
		int i=0, j=0;
		for(; i < nbFst; i++)
		{
			this->fstThrList[i]= i % nbPUNode;
		}
		for(; j < nbSlow; i++, j++)
		{
			this->slwThrList[j]=  i % nbPUNode;
		}
	}
	else
	{
		int nbFst = nbThread - nbSlow;
		this->fstThrList = new int[nbFst];
		this->slwThrList = new int[nbSlow];
		int i=0, j=0;
		for(; i < nbFst; i++)
		{
			this->fstThrList[i]= i % nbPUNode;
		}
		for(; j < nbSlow; j++)
		{
			this->slwThrList[j]= nbPUNode + j % nbPUNode;
		}
	}
}


void ThrParam::info()
{
		int nbFst = nbThread - nbSlow;
		cout << nbThread << " threads to be launched" << endl;
		if (nbFst > 0)
		{
			printf("Lauching %d fast threads\n", nbFst);
			for(int i = 0; i < nbFst; i++)
			{
				printf("fast thread %d will run on cpu %d \n",i, this->fstThrList[i]);
			}
		}
		if (this->nbSlow > 0)
		{
			printf("Lauching %d slow threads\n", nbSlow);
			for(int i = 0; i < nbSlow; i++)
			{
				printf("slow thread %d will run on cpu %d \n",i, this->slwThrList[i]);
			}
		}
}

ThrParam::~ThrParam()
{
	if (this->fstThrList != NULL)
		delete[] this->fstThrList;
	if (this->slwThrList != NULL)
		delete[] this->slwThrList;
}



// function for parsing main arguments
int parseArg(int argc, char * args[], ThrParam **param)
{
	int nbThread=-1, nbSlow=0;
	bool close=true;
	int * fstThr;
	int * slwThr;



	string arr[argc -1];
	for(int i = 1; i < argc; i++)
	{
		arr[i - 1] = string(args[i]);
	}

	const string str1 = "-n=";
	const string str2 = "-slow=";
	const string str3 = "-places=";
	const string cstr= "close"; 
	const string sstr= "spread"; 
	for (int i = 0; i < argc - 1; i++)
	{
		if (arr[i].compare(0, str1.size(), str1) == 0)
		{
			string nbstr= arr[i].substr(str1.size(), string::npos);
			nbThread= stoi(nbstr, NULL);
		}
		else if (arr[i].compare(0, str2.size(), str2) == 0)
		{
			string nbstr= arr[i].substr(str2.size(), string::npos);
			nbSlow= stoi(nbstr, NULL);
		}
		else if (arr[i].compare(0, str3.size(),str3) == 0)
		{
			string substr= arr[i].substr(str3.size(), string::npos);
			if (substr.compare(0, substr.size(), cstr) == 0)
				close= true;
			else if (substr.compare(0, substr.size(), sstr) == 0)
				close= false;
			else
			{
				cout << "wrong places, format is close | spread" << endl;
				return -1;
			}
		}
		else
			return -1;

	}

	if (nbThread > 0 && nbThread < nbSlow)
	{
		cout << "the number of threads must be greater or equal to the number of slow threads" << endl;
		return -1;
	}

	*param = new ThrParam(nbThread, nbSlow, close);
	if (*param == NULL)
		cout << "param not allocated in parse" << endl;

	return 0;
}

void parsePlaces(const string& str, int * tab)
{
	const string str1= "close"; 
	const string str2= "spread"; 
	if (str.compare(0, str1.size(),str1) == 0)
	{;}
}


int getCpuPerNode()
{
	unsigned depth, nbcpu, nbnode;
	hwloc_topology_t topo;

	depth = hwloc_topology_init(&topo);
	hwloc_topology_load(topo);

	nbnode = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NODE);
	nbcpu = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);

	printf("nbnode : %d, nbcpu : %d\n",nbnode,nbcpu);
	if (nbnode <= 0 && nbcpu > 0)
		//no numa node found
		return nbcpu;
	else if (nbcpu > 0)
		return nbcpu / nbnode;
	else
	{
		printf("failed to get topo\n");
		return -1;
	}
}

