#include "parser.h"

using namespace std;


ThrParam::ThrParam():nbThread(NCORES_PER_NODE),nbSlow(0),close(true), slwThrList(NULL)
{
	this->fstThrList = new int[NCORES_PER_NODE] ;
	for(int i = 0; i < NCORES_PER_NODE; i++)
		this->fstThrList[i] = i;
}

ThrParam::ThrParam(int nbT, int nbSlw):nbThread(nbT),nbSlow(nbSlw),close(true)
{
	if (nbSlw == nbT)
	{
		this->fstThrList = NULL;
		this->slwThrList = new int[nbSlw];
		for(int i = 0; i < nbThread; i++)
			// take modulo NCORES_PER_NODE cause we have NCORES_PER_NODE cores
			this->slwThrList[i] = i % NCORES_PER_NODE;
	}
	else
	{
		int nbFst = nbThread - nbSlow;
		this->fstThrList = new int[nbFst];
		this->slwThrList = new int[nbSlw];
		int i=0, j=0;
		for(; i < nbFst; i++)
		{
			this->fstThrList[i]= i % NCORES_PER_NODE;
		}
		printf("i = %d\n", i);
		for(; j < nbSlw; i++, j++)
		{
			printf("slow thread %d placed on cpu %d\n", j, i % NCORES_PER_NODE);
			this->slwThrList[j]= i % NCORES_PER_NODE;
		}

	}

}

ThrParam::ThrParam(int nbT, int nbSlw, bool close):nbThread(nbT),nbSlow(nbSlw), close(close)
{
	if (nbSlw == nbT)
	{
		this->fstThrList = NULL;
		this->slwThrList = new int[nbSlw];
		for(int i = 0; i < nbThread; i++)
			// take modulo NCORES_PER_NODE cause we have NCORES_PER_NODE cores
			this->slwThrList[i] = i % NCORES_PER_NODE;
	}
	else if(close)
		//case we want to place every
		//thread on a node (same cache)
	{
		int nbFst = nbThread - nbSlow;
		this->fstThrList = new int[nbFst];
		this->slwThrList = new int[nbSlw];
		int i=0, j=0;
		for(; i < nbFst; i++)
		{
			this->fstThrList[i]= i % NCORES_PER_NODE;
		}
		for(; j < nbSlw; i++, j++)
		{
			this->slwThrList[j]=  i % NCORES_PER_NODE;
		}
	}
	else
	{
		int nbFst = nbThread - nbSlow;
		this->fstThrList = new int[nbFst];
		this->slwThrList = new int[nbSlw];
		int i=0, j=0;
		for(; i < nbFst; i++)
		{
			this->fstThrList[i]= i % NCORES_PER_NODE;
		}
		for(; j < nbSlw; j++)
		{
			this->slwThrList[j]= NCORES_PER_NODE + j % NCORES_PER_NODE;
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
	int nbThread=NCORES_PER_NODE, nbSlow=0;
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

	if (nbThread < nbSlow)
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
