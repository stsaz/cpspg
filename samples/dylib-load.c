/* Cross-Platform System Programming Guide: L2: call a function from dynamic library */

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

#define DL_EXT  "dll"
typedef HMODULE dl_t;
#define DL_NULL  NULL

dl_t dl_open(const char *filename, unsigned int flags)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, filename, -1, w, 1000))
		return INVALID_HANDLE_VALUE;

	return LoadLibraryExW(w, NULL, flags);
}

void dl_close(dl_t dl)
{
	FreeLibrary(dl);
}

void* dl_addr(dl_t dl, const char *proc_name)
{
	return (void*)GetProcAddress(dl, proc_name);
}

const char* err_strptr(int code)
{
	static char buf[1000];
	buf[0] = '\0';

	wchar_t w[250];
	unsigned int flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
	int n = FormatMessageW(flags, 0, code, 0, w, 250, 0);
	if (n == 0)
		return buf;

	WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, sizeof(buf), NULL, NULL);
	return buf;
}

const char* dl_errstr()
{
	return err_strptr(GetLastError());
}

#else // UNIX:

#include <dlfcn.h>

#if defined __APPLE__ && defined __MACH__
	#define DL_EXT  "dylib"
#else
	#define DL_EXT  "so"
#endif

typedef void* dl_t;
#define DL_NULL  NULL

dl_t dl_open(const char *filename, unsigned int flags)
{
	return dlopen(filename, flags | RTLD_LAZY);
}

void dl_close(dl_t dl)
{
	dlclose(dl);
}

void* dl_addr(dl_t dl, const char *proc_name)
{
	return dlsym(dl, proc_name);
}

const char* dl_errstr()
{
	return dlerror();
}

#endif

typedef void (*func_t)();

void main()
{
	// open library file
	dl_t dl = dl_open("./dylib." DL_EXT, 0);
	assert(dl != DL_NULL);

	// get address of the function exported by the library
	func_t func = dl_addr(dl, "func");

	// call the function provided by dynamic library
	func();

	dl_close(dl);

	// try to open a file which doesn't exist
	dl = dl_open("./abc", 0);
	assert(dl == DL_NULL);

	// print error message
	puts(dl_errstr());
}
