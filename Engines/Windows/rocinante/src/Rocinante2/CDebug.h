#include <stdio.h>
#ifdef _DEBUG 
#include <assert.h>
 
#define ASSERT(x) assert((x))

#else
#define ASSERT(x);
#endif
