
#ifndef EGBB_H
#define EGBB_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#include <dlfcn.h>
#define EGBB_NAME       "egbb.dylib"
#define EGBB_NAME2      "egbbdylib.dylib"
#define EGBB64_NAME     "egbb64.dylib"
#define EGBB64_NAME2    "egbbdylib64.dylib"
#define load_library(x) dlopen((x), RTLD_LAZY)
#define load_function   dlsym
#define unload_library  dlclose
#define lib_handle_t    void*
#define CDECL
#endif

#ifdef __linux__
#include <dlfcn.h>
#define EGBB_NAME       "egbb.so"
#define EGBB_NAME2      "egbbso.so"
#define EGBB64_NAME     "egbb64.so"
#define EGBB64_NAME2    "egbbso64.so"
#define load_library(x) dlopen((x), RTLD_LAZY)
#define load_function   dlsym
#define unload_library  dlclose
#define lib_handle_t    void*
#define CDECL
#endif

#ifdef _WIN32
#define EGBB_NAME       "egbb.dll"
#define EGBB_NAME2      "egbbdll.dll"
#define EGBB64_NAME     "egbb64.dll"
#define EGBB64_NAME2    "egbbdll64.dll"
#define load_library    LoadLibrary
#define load_function   GetProcAddress
#define unload_library  FreeLibrary
#define lib_handle_t    HMODULE
#endif

#define SCORPIO_BB_WIN_SCORE  5000

#ifdef __cplusplus
} // extern "C"
#endif
#endif // EGBB_H
