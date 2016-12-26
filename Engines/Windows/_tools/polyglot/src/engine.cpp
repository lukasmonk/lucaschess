#ifndef _WIN32

// engine.cpp

// includes

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <unistd.h>

#include "engine.h"
#include "io.h"
#include "option.h"
#include "util.h"


// constants

static const int StringSize = 4096;

// variables

engine_t Engine[1];

// prototypes

static void my_close (int fd);
static void my_dup2  (int old_fd, int new_fd);

// functions

// engine_is_ok()

bool engine_is_ok(const engine_t * engine) {

   if (engine == NULL) return false;

   if (!io_is_ok(engine->io)) return false;

   return true;
}

// engine_open()

void engine_open(engine_t * engine) {

   const char * dir, * command;
   char string[StringSize];
   int argc;
   char * ptr;
   char * argv[256];
   int from_engine[2], to_engine[2];
   pid_t pid;

   ASSERT(engine!=NULL);

   // init

   dir = option_get_string("EngineDir");
   my_log("POLYGLOT Dir \"%s\"\n",dir);

   command = option_get_string("EngineCommand");
   my_log("POLYGLOT Command \"%s\"\n",command);

   // parse the command line and create the argument list

   if (strlen(command) >= StringSize) my_fatal("engine_open(): buffer overflow\n");
   strcpy(string,command);

   argc = 0;

   for (ptr = strtok(string," "); ptr != NULL; ptr = strtok(NULL," ")) {
      argv[argc++] = ptr;
   }

   argv[argc] = NULL;

   // create the pipes

   if (pipe(from_engine) == -1) {
      my_fatal("engine_open(): pipe(): %s\n",strerror(errno));
   }

   if (pipe(to_engine) == -1) {
      my_fatal("engine_open(): pipe(): %s\n",strerror(errno));
   }

   // create the child process

   pid = fork();

   if (pid == -1) {

      my_fatal("engine_open(): fork(): %s\n",strerror(errno));

   } else if (pid == 0) {

      // child = engine

      // close unused pipe descriptors to avoid deadlocks

      my_close(from_engine[0]);
      my_close(to_engine[1]);

      // attach the pipe to standard input

      my_dup2(to_engine[0],STDIN_FILENO);
      my_close(to_engine[0]);

      // attach the pipe to standard output

      my_dup2(from_engine[1],STDOUT_FILENO);
      my_close(from_engine[1]);

      // attach standard error to standard output

      my_dup2(STDOUT_FILENO,STDERR_FILENO);

      // set a low priority

      if (option_get_bool("UseNice"))
      {
          my_log("POLYGLOT Adjust Engine Piority");
          nice(+option_get_int("NiceValue"));
      }

      // change the current directory

      if (dir[0] != '\0' && chdir(dir) == -1) {
         my_fatal("engine_open(): chdir(): %s\n",strerror(errno));
      }

      // launch the new executable file

      execvp(argv[0],&argv[0]);

      // execvp() only returns when an error has occured

      my_fatal("engine_open(): execvp(): %s\n",strerror(errno));

   } else { // pid > 0

      ASSERT(pid>0);

      // parent = PolyGlot

      // close unused pipe descriptors to avoid deadlocks

      my_close(from_engine[1]);
      my_close(to_engine[0]);

      // fill in the engine struct

      engine->io->in_fd = from_engine[0];
      engine->io->out_fd = to_engine[1];
      engine->io->name = "Engine";

      io_init(engine->io);
   }
}

// engine_close()

void engine_close(engine_t * engine) {

   ASSERT(engine_is_ok(engine));

   io_close(engine->io);
}

// engine_get()

void engine_get(engine_t * engine, char string[], int size) {

   ASSERT(engine_is_ok(engine));
   ASSERT(string!=NULL);
   ASSERT(size>=256);

   while (!io_line_ready(engine->io)) {
      io_get_update(engine->io);
   }

   if (!io_get_line(engine->io,string,size)) { // EOF
       my_log("POLYGLOT *** EOF from Engine ***\n");
      exit(EXIT_SUCCESS);
   }
}

// engine_send()

void engine_send(engine_t * engine, const char format[], ...) {

   va_list arg_list;
   char string[StringSize];

   ASSERT(engine_is_ok(engine));
   ASSERT(format!=NULL);

   // format

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);

   // send

   io_send(engine->io,"%s",string);
}

// engine_send_queue()

void engine_send_queue(engine_t * engine, const char format[], ...) {

   va_list arg_list;
   char string[StringSize];

   ASSERT(engine_is_ok(engine));
   ASSERT(format!=NULL);

   // format

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);

   // send

   io_send_queue(engine->io,"%s",string);
}

// my_close()

static void my_close(int fd) {

   ASSERT(fd>=0);

   if (close(fd) == -1) my_fatal("my_close(): close(): %s\n",strerror(errno));
}

// my_dup2()

static void my_dup2(int old_fd, int new_fd) {

   ASSERT(old_fd>=0);
   ASSERT(new_fd>=0);

   if (dup2(old_fd,new_fd) == -1) my_fatal("my_dup2(): dup2(): %s\n",strerror(errno));
}

// end of engine.cpp
#else
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include "engine.h"
#include "option.h"
#include "pipe.h"

PipeStruct pipeStdin, pipeEngine;
static const int StringSize = 4096;
static int nQueuePtr = 0;
static char szQueueString[StringSize];
engine_t Engine[1];

static HANDLE child;
static HANDLE Engine_In;


DWORD GetWin32Priority(int nice)
{
/*
REALTIME_PRIORITY_CLASS     0x00000100
HIGH_PRIORITY_CLASS         0x00000080
ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
NORMAL_PRIORITY_CLASS       0x00000020
BELOW_NORMAL_PRIORITY_CLASS 0x00004000
IDLE_PRIORITY_CLASS         0x00000040
*/
	if (nice < -15) return 0x00000080;
	if (nice < 0)   return 0x00008000;
	if (nice == 0)  return 0x00000020;
	if (nice < 15)  return 0x00004000;
	return 0x00000040;
}

void PipeStruct::Open(const char *szProcFile) {
  DWORD dwMode;
  HANDLE hStdinRead, hStdinWrite, hStdoutRead, hStdoutWrite;
  SECURITY_ATTRIBUTES sa;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  if (szProcFile == NULL) {
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    bConsole = GetConsoleMode(hInput, &dwMode);
  } else {
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0);
    CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0);
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = si.lpDesktop = si.lpTitle = NULL;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;
    si.hStdInput = hStdinRead;
    si.hStdOutput = hStdoutWrite;
    si.hStdError = hStdoutWrite;
    if(CreateProcess(NULL, (LPSTR) szProcFile, NULL, NULL, TRUE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi)==0)
	{
		//failed
		printf("tellusererror Polyglot: EngineCommand error: %s\n",szProcFile);
		fflush(stdout);
		Sleep(50000);
		my_fatal("Can't open engine %s\n",szProcFile);
	}
	child=pi.hProcess;
    CloseHandle(pi.hThread);
    CloseHandle(hStdinRead);
    CloseHandle(hStdoutWrite);
    Engine_In=hInput = hStdoutRead;
    hOutput = hStdinWrite;
    bConsole = FALSE;
  }
  if (bConsole) {
    SetConsoleMode(hInput, dwMode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
    FlushConsoleInputBuffer(hInput);
  } else {
    nBytesLeft = 0;
  }
  nReadEnd = 0;
}


void PipeStruct::Close(void) const {
  CloseHandle(hInput);
  CloseHandle(hOutput);
  DWORD lpexit;

  my_log("closing child\n");
  if(GetExitCodeProcess(child,&lpexit)){
	if(lpexit==STILL_ACTIVE)
	  my_log("process still active after \"quit\" ");
	//must be java,hammer it down!
	TerminateProcess(child,lpexit);
  }
	CloseHandle(child);
}

void PipeStruct::ReadInput(void) {
  DWORD dwBytes;
  ReadFile(hInput, szBuffer + nReadEnd, LINE_INPUT_MAX_CHAR - nReadEnd, &dwBytes, NULL);
  nReadEnd += dwBytes;
  if (nBytesLeft > 0) {
    nBytesLeft -= dwBytes;
  }
}

void engine_get(engine_t * engine, char *szLineStr, int size){
	char string;//a short one
	char *p=szLineStr;
	BOOL result;
	DWORD dwBytes;

	int n=0;//null chars

	while(1){
		result=ReadFile(Engine_In,&string,1,&dwBytes,NULL);
		//if(result==false) my_log("readfile failed dwbytes %d\n",dwBytes);
		//this happens btw if the engine got closed
		if (string =='\r' || string == '\n'){
			if(n>0){//we got something
				*p='\0';
				my_log("Engine->Adapter: %s\n", szLineStr);
				return;
			}
			continue;//ignore eol 
		}
		*p++=string;
		n++;
	}
}


Boolean PipeStruct::CheckInput(void) {
  DWORD dwEvents, dwBytes;
  if (bConsole) { // a tty, or an un-redirected handle
    GetNumberOfConsoleInputEvents(hInput, &dwEvents);
    if (dwEvents > 1) {
      return TRUE;
    } else {
      return FALSE;
    }
  } else { // a handle redirected to a pipe or a file
    if (nBytesLeft > 0) {
      return TRUE;
    } else {
      if (PeekNamedPipe(hInput, NULL, 0, NULL, &dwBytes, NULL)) {
        nBytesLeft = dwBytes;
        return nBytesLeft > 0; // a pipe
      } else {
        return TRUE; // a file, always TRUE
      }
    }
  }
}

void PipeStruct::LineOutput(const char *szLineStr) const {
  DWORD dwBytes;
  size_t nStrLen;
  char szWriteBuffer[LINE_INPUT_MAX_CHAR];
  nStrLen = strlen(szLineStr);
  memcpy(szWriteBuffer, szLineStr, nStrLen);
  szWriteBuffer[nStrLen] = '\r';
  szWriteBuffer[nStrLen + 1] = '\n';
  WriteFile(hOutput, szWriteBuffer, (DWORD)(nStrLen + 2), &dwBytes, NULL);
}

Boolean PipeStruct::GetBuffer(char *szLineStr) {
  char *lpFeedEnd;
  size_t nFeedEnd;
  lpFeedEnd = (char *) memchr(szBuffer, '\n', nReadEnd);
  if (lpFeedEnd == NULL) {
    return FALSE;
  } else {
    nFeedEnd = (lpFeedEnd - szBuffer);
    memcpy(szLineStr, szBuffer, nFeedEnd);
    if (szLineStr[nFeedEnd - 1] == '\r') {
      szLineStr[nFeedEnd - 1] = '\0';
    } else {
      szLineStr[nFeedEnd] = '\0';
    }
    nFeedEnd ++;
    nReadEnd -= (int)nFeedEnd;
    memcpy(szBuffer, szBuffer + nFeedEnd, nReadEnd);
    return TRUE;
  }
}

Boolean PipeStruct::LineInput(char *szLineStr) {
  if (GetBuffer(szLineStr)) {
    return TRUE;
  }
  if (CheckInput()) {
    ReadInput();
    if (GetBuffer(szLineStr)) {
      return TRUE;
    } else {
      if (nReadEnd == LINE_INPUT_MAX_CHAR) {
        memcpy(szLineStr, szBuffer, LINE_INPUT_MAX_CHAR - 1);
        szLineStr[LINE_INPUT_MAX_CHAR - 1] = '\0';
        szBuffer[0] = szBuffer[LINE_INPUT_MAX_CHAR - 1];
        nReadEnd = 1;
        return TRUE;
      } else {
        return FALSE;
      }
    }
  } else {
    return FALSE;
  }
}


void engine_close(engine_t *engine){
	pipeEngine.Close();
}

void engine_open(engine_t * engine){
   int affinity;
    char *my_dir;
    if( (my_dir = _getcwd( NULL, 0 )) == NULL )
        my_fatal("Can't build path: %s\n",strerror(errno));
	if(SetCurrentDirectory(option_get_string("EngineDir"))==0){
		printf("tellusererror Polyglot:EngineDir error: %s\n",option_get_string("EngineDir"));
		fflush(stdout);
		Sleep(50000);
		my_fatal("EngineDir path error: %s\n",option_get_string("EngineDir"));
	}
	pipeEngine.Open(option_get_string("EngineCommand"));
        //play with affinity
#ifndef NO_AFFINITY
    affinity=option_get_int("Affinity");

	if(affinity!=-1){
		my_log("POLYGLOT SetProcess Affinity\n");
		SetProcessAffinityMask(child,affinity); //
	}
#endif
        //lets go back
    SetCurrentDirectory(my_dir);
        // set priority
    if (option_get_bool("UseNice")){
          my_log("POLYGLOT Adjust Engine Piority\n");
		  SetPriorityClass(child, GetWin32Priority(option_get_int("NiceValue")));
    }
}

void engine_send_queue(engine_t * engine,const char *szFormat, ...) {
  nQueuePtr += vsprintf(szQueueString + nQueuePtr, szFormat, (va_list) (&szFormat + 1));
}

void engine_send(engine_t * engine, const char *szFormat, ...) {
  vsprintf(szQueueString + nQueuePtr, szFormat, (va_list) (&szFormat + 1));
  pipeEngine.LineOutput(szQueueString);
  my_log("Adapter->Engine: %s\n", szQueueString);
  nQueuePtr = 0;
}

Boolean peek_engine_get(engine_t * engine, char *szLineStr, int size){
	if (pipeEngine.LineInput(szLineStr)) {
    my_log("Engine->Adapter: %s\n", szLineStr);
    return TRUE;
  } else {
	szLineStr[0]='\0';
    return FALSE;
  }
}

#endif
