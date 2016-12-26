#ifndef PIPE_H
#define PIPE_H

#ifndef BOOLEAN_H
#define BOOLEAN_H


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef char BoolChar;        // bc
typedef short BoolShort;      // bw
typedef long BoolLong;        // bdw
typedef __int64 BoolLongLong; // bqw

typedef int Boolean;          // b

inline Boolean Eqv(Boolean bArg1, Boolean bArg2) {
  return bArg1 ? bArg2 : !bArg2;
}

inline Boolean Xor(Boolean bArg1, Boolean bArg2) {
  return bArg1 ? !bArg2 : bArg2;
}

#endif

//#define PIPE_H

const int LINE_INPUT_MAX_CHAR = 4096;

struct PipeStruct {
  HANDLE hInput, hOutput;
  BOOL bConsole;
  int nBytesLeft;
  int nReadEnd;
  char szBuffer[LINE_INPUT_MAX_CHAR];

  void Open(const char *szExecFile = NULL);
  void Close(void) const;
  void ReadInput(void);
  Boolean CheckInput(void);
  Boolean GetBuffer(char *szLineStr);
  Boolean LineInput(char *szLineStr);
  void LineOutput(const char *szLineStr) const;
}; // pipe


extern PipeStruct pipeStdin, pipeEngine;
#endif // else