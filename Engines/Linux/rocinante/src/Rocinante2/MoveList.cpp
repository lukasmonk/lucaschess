#include "CDebug.h"
#include "MoveList.h"

MoveList::MoveList(void)
{
	size = 0;
}

MoveList::~MoveList(void)
{
}

void MoveList::Remove(int pos)
{
   int i;

   ASSERT(IsOk());
   ASSERT(pos>=0&&pos<size);

   for (i = pos; i < size-1; i++) {
      move[i] = move[i+1];
      value[i] = value[i+1];
   }
   size--;
}


void MoveList::Sort()
{
	int i,j,max;
	int pivote;
	// normalmente hay muchos 0 agrupamos delante asi la reordenación es menor
	j = 0;
	while(value[j] > 0)j++;
	for(i=j+1;i < size-1;i++)
	{
		if(value[i] >0)
		{
			value[j] = value[i];
			value[i] = 0;
			pivote = move[j];
			move[j] = move[i];
			move[i] = pivote;
			j++;
		}
	}
	max = j;
	for(i=0; i < max-1;i++)
	{
		for(j=i+1; j < size;j++)
		{
			if(value[j] > value[i])
			{
				pivote = value[i];
				value[i] = value[j];
				value[j] = pivote;
				pivote = move[i];
				move[i] = move[j];
				move[j] = pivote;
			}
		}
	}
}
