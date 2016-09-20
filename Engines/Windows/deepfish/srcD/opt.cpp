/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright(C)2004-2008 Tord Romstad (Glaurung author)
  Copyright(C)2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation,either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

#include "thread.h"
#include "tt.h"

using namespace std;

#define TRUE 1
#define FALSE 0

#define MEMALIGN(a, b, c) a = _aligned_malloc (c, b) 
#define ALIGNED_FREE(x) _aligned_free (x)

int large_use;

#ifndef _WIN32 // Linux 
#include <sys/ipc.h>
#include <sys/shm.h>

static int num;

void SETUP_PRIVILEGES(){}

void CREATE_MEM(void** A,int align,uint64_t size)
{
	large_use=FALSE;

	num=shmget(IPC_PRIVATE,size,IPC_CREAT|SHM_R|SHM_W|SHM_HUGETLB);
	if(num==-1)
	{
		printf("info string LargePages FAILED %llu Mb\n",size>>20);
		MEMALIGN((*A),align,size);
	}
	else
	{
		(*A)=shmat(num,NULL,0x0);
		large_use=TRUE;
		printf("info string LargePages OK %llu Mb\n",size>>20);
		std::cout<<"info string HUGELTB "<<(size>>20)<<std::endl;
	}
}

void CREATE_MEM2(void** A,uint64_t size)
{
	large_use=FALSE;
	num=shmget(IPC_PRIVATE,size,IPC_CREAT|SHM_R|SHM_W|SHM_HUGETLB);
	if(num!=-1)
	{
		(*A)=shmat(num,NULL,0x0);
		large_use=TRUE;
		printf("info string %llu Mb LargePages\n",size>>20);
	}
}

void FREE_MEM(void* A)
{
	if(!A)
		return;
	if(!large_use)
	{
		ALIGNED_FREE(A);
		return;
	}
	shmdt(A);
	shmctl(num,IPC_RMID,NULL);
	large_use=FALSE;
}

void SETUP_PRIVILEGES(){}

#else

void CREATE_MEM(void** A,int align,uint64_t size)
{
	large_use=FALSE;
	(*A)=VirtualAlloc(NULL,size,MEM_LARGE_PAGES|MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
	if((*A))
	{
		large_use=TRUE;
		printf("info string %llu Mb LargePages\n",size>>20);
	}
	else
	{
		printf("info string %llu Mb(no LargePages)\n",size>>20);
		MEMALIGN((*A),align,size);
	}
}

void CREATE_MEM2(void** A,uint64_t size)
{
	large_use=FALSE;
	(*A)=VirtualAlloc(NULL,size,MEM_LARGE_PAGES|MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
	if((*A))
	{
		large_use=TRUE;
		printf("info string LargePages %llu Mb\n",size>>20);
	}
}

void FREE_MEM(void* A)
{
	if(!A)
		return;
	if(!large_use)
		ALIGNED_FREE(A);
	else
	{
		VirtualFree(A,0,MEM_RELEASE);
		large_use=FALSE;
	}
}

void SETUP_PRIVILEGES()
{
	HANDLE TH,PROC;
	TOKEN_PRIVILEGES tp;

	PROC=GetCurrentProcess();
	OpenProcessToken(PROC,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&TH);
	LookupPrivilegeValue(NULL,TEXT("SeLockMemoryPrivilege"),&tp.Privileges[0].Luid);
	tp.PrivilegeCount=1;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(TH,FALSE,&tp,0,NULL,0);
	CloseHandle(TH);
}
#endif
