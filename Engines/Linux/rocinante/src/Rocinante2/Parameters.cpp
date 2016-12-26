#include <stdio.h>
#include "Parameters.h"
#include <string.h>
#include <stdlib.h>

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
bool LoadParam(char *name)
{
	char buffer[128];
	int j;
	char ParamName[128];
	int value;

	FILE *fdo = fopen(name,"r+");
	if(fdo)
	{
		while(fgets(buffer,sizeof(buffer),fdo))
		{
			if(strlen(buffer) > 10)
			{
				if(buffer[5] == '(')
				{
					buffer[5] = '\0';
					if(strcmp(buffer,"Param") == 0)
					{
						// starts with Param( 
						for(j=0; buffer[j+6] != ',';j++)
						{
							ParamName[j] = buffer[j+6];
						}
						ParamName[j] = '\0';
						j++;
						value = atoi(&buffer[j+6]);
						// now search the param position
						for(j=0;j <  MaxParameters;j++)
						{
							if(strcmp(ParamName,ParametersN[j]) == 0)
							{
								Parameters[j] = value;
								break;
							}
						}
					}
				}
			}
		}
		fclose(fdo);
		return true;
	}
	return false;
}
