/* Cross-Platform System Programming Guide: L2: dynamic library */

#include <stdio.h>

#ifdef _WIN32
	#define EXPORT  __declspec(dllexport)
#else
	#define EXPORT  __attribute__((visibility("default")))
#endif

EXPORT void func()
{
	puts("Hello from dynamic library");
}
