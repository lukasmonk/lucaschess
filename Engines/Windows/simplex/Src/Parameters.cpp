#include <stdio.h>
#include "Parameters.h"

// make name array of Param
#undef Param
#define Param(n,v)	#n
const char * ParametersN[MaxParameters] ={
#include "Param.h"
};

void SaveParam(char *name)
{
	char buffer[128];
	int j;

	FILE *fdo = fopen(name,"w+");
	if(fdo)
	{
		for(j=0;j <  MaxParameters;j++)
		{
			sprintf(buffer,"Param(%s,%d),\n",ParametersN[j],Parameters[j]);
			fputs(buffer,fdo);
		}
		fclose(fdo);
	}
}
