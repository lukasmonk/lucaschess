#define Param(n,v) n
enum ParamName {
#include "Param.h"
	MaxParameters
};
#undef Param

//const int MaxParameters = MAXPARAMETERS;
extern short Parameters[MaxParameters];
extern char * ParametersN[MaxParameters];

// Save parameters to a file Param.h  (tuning utility)
extern void SaveParam(char *name);
extern bool LoadParam(char *name);

