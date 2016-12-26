#include "C3FoldRep.h"
#include <string.h>

C3FoldRep ThreeFold;

static char Pos[600][100];
static int actual;

C3FoldRep::C3FoldRep(void)
{
	Reset();
}

C3FoldRep::~C3FoldRep(void)
{
}

void C3FoldRep::Reset()
{
	actual = 0;
}
void C3FoldRep::Add(char *fen)
{
	strcpy(&Pos[actual++][0],fen);
}
bool C3FoldRep::IsRep(char *fen)
{
	int i;
	if(!fen) return false;
	if(*fen == '\0') return false;
	if(actual==0) return false;
	for(i = actual-1;i>= 0;i--)
	{
		if(strcmp(&Pos[i][0],fen) ==0 )
			return true;
	}
	return false;
}
