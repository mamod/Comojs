#include "../include/duktape/duktape.h"

#ifdef __cplusplus
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif

#ifdef _WIN32
	#define COMO_MODULE_INIT() EXTERNC __declspec(dllexport) void init(duk_context *ctx)
#else
	#define COMO_MODULE_INIT() EXTERNC void init(duk_context *ctx)
#endif

#ifndef _WIN32
	#include <dlfcn.h>
#else
	#include <windows.h>
	#define dlsym(x, y) GetProcAddress((HMODULE)x, y)
#endif
