// Parameters are defined in the include Param.h
// like Param(paramName,value)
// the macro Param is redefined on three flavors: 
// an enum for index position (Parameters.h), a parameter array (BEval.cpp) and an array of param names(Parameters.cpp).

// make enum for index position
#define Param(n,v) n
enum ParamName {
#include "Param.h"
	MaxParameters		// max index for all loops on Parameters
};
#undef Param

extern short Parameters[MaxParameters];
extern const char * ParametersN[MaxParameters];

// Save parameters to a Param.h file (tuning utility)
extern void SaveParam(char *name);

