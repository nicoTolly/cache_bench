#include "parser.h"

using namespace std;

class myexception: public exception
{
  virtual const char* what() const throw()
  {
    return "The number of threads must be greater or equal to the number of slow threads";
  }
} myex;

ThrParam::ThrParam():fsiz(NDEFAULT), ssiz(NDEFAULT),nbSlow(0),close(true)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCPU_PER_SOCK;
	else
		nbPUNode = res;
	nbThread = nbPUNode;
	this->init();
}

ThrParam::ThrParam(int nbT, int nbSlw):fsiz(NDEFAULT),ssiz(NDEFAULT), nbSlow(nbSlw),close(true)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCPU_PER_SOCK;
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

ThrParam::ThrParam(int nbT, int nbSlw, bool close):fsiz(NDEFAULT),ssiz(NDEFAULT), nbSlow(nbSlw), close(close)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCPU_PER_SOCK;
	else
		nbPUNode = res;
	if (nbT == -1)
		nbThread = nbPUNode;
	else
		nbThread = nbT;
	this->init();
}

ThrParam::ThrParam(int nbT, int nbSlw, bool close, int siz):fsiz(siz), ssiz(siz), nbSlow(nbSlw), close(close)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCPU_PER_SOCK;
	else
		nbPUNode = res;
	if (nbT == -1)
		nbThread = nbPUNode;
	else
		nbThread = nbT;
	this->init();
}

ThrParam::ThrParam(int nbT, int nbSlw, bool close, int fsiz, float ratio):fsiz(fsiz), nbSlow(nbSlw), close(close)
{
	int res;

	if((res = getCpuPerNode()) < 0)
		nbPUNode = NCPU_PER_SOCK;
	else
		nbPUNode = res;
	if (nbT == -1)
		nbThread = nbPUNode;
	else
		nbThread = nbT;
	// compute slow threads array size
	// from fast thread array size
	// must guarantee alignment on 32 bytes
	ssiz =(int) (ratio * fsiz) & ~0x3;	
	this->init();
}

void ThrParam::init()
{
	if(nbSlow > nbThread)
	{
		throw myex;
	}
	else if (nbSlow == nbThread)
	{
		this->fstThrList = NULL;
		this->slwThrList = new int[nbSlow];
		for(int i = 0; i < nbThread; i++)
			// take modulo NCPU_PER_SOCK cause we have NCPU_PER_SOCK cores
			this->slwThrList[i] = i % nbPUNode;
	}
	else if (nbSlow == 0)
	{
		this->slwThrList = NULL;
		this->fstThrList = new int[nbThread];
		for(int i = 0; i < nbThread; i++)
			// take modulo NCPU_PER_SOCK cause we have NCPU_PER_SOCK cores
			this->fstThrList[i] = i % nbPUNode;
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
	this->globsiz = this->ssiz + this->fsiz;
}


void ThrParam::info()
{
		int nbFst = nbThread - nbSlow;
		cout << nbThread << " threads to be launched" << endl;
		cout << fsiz << " double per fast thread" << endl;
		cout << ssiz << " double per slow thread" << endl;
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
	int siz = NDEFAULT;
	float ratio = 1;



	string arr[argc -1];
	for(int i = 1; i < argc; i++)
	{
		arr[i - 1] = string(args[i]);
	}

	const string str1 = "-n=";
	const string str2 = "-slow=";
	const string str3 = "-places=";
	const string str4 = "-size=";
	const string str5 = "-ratio=";
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
		else if (arr[i].compare(0, str4.size(), str4) == 0)
		{
			string nbstr= arr[i].substr(str4.size(), string::npos);
			int a= stoi(nbstr, NULL);
			siz = 1 << a;
		}
		else if (arr[i].compare(0, str5.size(), str5) == 0)
		{
			string nbstr= arr[i].substr(str5.size(), string::npos);
			 ratio= stof(nbstr, NULL);
		}
		else
			return -1;

	}


	try
	{
		*param = new ThrParam(nbThread, nbSlow, close, siz, ratio);
	}
	// case nbThread > nbSlow
	catch (exception& e)
	{
		cout << e.what() << endl;
		exit(EXIT_FAILURE);
	}
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
#ifdef HWLOC
	unsigned depth, nbcpu, nbsock;
	hwloc_topology_t topo;

	depth = hwloc_topology_init(&topo);
	hwloc_topology_load(topo);

	nbsock = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_SOCKET);
	nbcpu = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);

	printf("nbsock : %d, nbcpu : %d\n",nbsock,nbcpu);
	if (nbsock <= 0 && nbcpu > 0)
		//no numa node found
		return nbcpu;
	else if (nbcpu > 0)
		return nbcpu / nbsock;
	else
	{
		printf("failed to get topo\n");
		return -1;
	}
#else
	printf("HWLOC not defined \n");
	return -1;
#endif
}

