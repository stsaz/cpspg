/* Cross-Platform System Programming Guide: L2: print current process's info */

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

unsigned int ps_curid()
{
	return GetCurrentProcessId();
}

void ps_exit(int code)
{
	ExitProcess(code);
}

const char* ps_filename(char *name, size_t cap)
{
	wchar_t w[1000];
	size_t n = GetModuleFileNameW(NULL, w, 1000);
	if (n == 0 || n == 1000)
		return NULL;

	if (!WideCharToMultiByte(CP_UTF8, 0, w, -1, name, cap, NULL, NULL))
		return NULL;

	return name;
}

int ps_curdir(char *buf, size_t cap)
{
	wchar_t w[1000];
	size_t n = GetCurrentDirectoryW(1000, w);
	if (n == 0)
		return -1;

	if (!WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, cap, NULL, NULL))
		return -1;
	return 0;
}

#else // UNIX:

#if defined __unix__ && !defined __linux__
	#include <sys/param.h>
#endif

#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#if defined __APPLE__ && defined __MACH__
	#include <mach-o/dyld.h>
#endif

unsigned int ps_curid()
{
	return getpid();
}

void ps_exit(int code)
{
	_exit(code);
}

/* Note: $PATH should be used to find a file in case argv0 is without path, e.g. "binary" */
const char* _path_real(char *name, size_t cap, const char *argv0)
{
	char real[PATH_MAX];
	if (NULL == realpath(argv0, real))
		return NULL;

	unsigned int n = strlen(real) + 1;
	if (n > cap)
		return NULL;

	memcpy(name, real, n);
	return name;
}

const char* ps_filename(char *name, size_t cap)
{
#if defined __linux__

	int n = readlink("/proc/self/exe", name, cap);
	if (n < 0)
		return NULL;
	name[n] = '\0';
	return name;

#elif defined BSD

#ifdef KERN_PROC_PATHNAME
	static const int sysctl_pathname[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, /*PID=*/ -1 };
	size_t namelen = cap;
	if (0 == sysctl(sysctl_pathname, 4, name, &namelen, NULL, 0))
		return name;
#endif

	int n = readlink("/proc/curproc/file", name, cap);
	if (n >= 0) {
		name[n] = '\0';
		return name;
	}

	n = readlink("/proc/curproc/exe", name, cap);
	if (n >= 0) {
		name[n] = '\0';
		return name;
	}

	return NULL;

#elif defined __APPLE__ && defined __MACH__

	unsigned int n = cap;
	if (0 == _NSGetExecutablePath(name, &n))
		return name;

	return NULL;

#endif
}

int ps_curdir(char *buf, size_t cap)
{
	return (NULL == getcwd(buf, cap));
}

#endif

void main()
{
	// get current PID
	unsigned int pid = ps_curid();
	printf("PID: %u\n", pid);

	// get current process executable file path
	char buf[1000];
	const char *fn = ps_filename(buf, sizeof(buf));
	assert(fn != NULL);
	printf("executable file name: %s\n", fn);

	// get current process directory
	assert(0 == ps_curdir(buf, sizeof(buf)));
	printf("current directory: %s\n", buf);

	// exit the process with specific code
	ps_exit(33);
}
