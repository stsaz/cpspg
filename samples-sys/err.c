/* Cross-Platform System Programming Guide: L1: print some system error message to console */

#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE file;
#define FILE_NULL  INVALID_HANDLE_VALUE

int file_close(file f)
{
	return !CloseHandle(f);
}

int err_last()
{
	return GetLastError();
}

const char* err_strptr(int code)
{
	// We need a static buffer so we can safely return its address to the user.
	// However, this approach isn't very good and must be used with care.
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

#else // UNIX:

#include <errno.h>
#include <string.h>
#include <unistd.h>

typedef int file;
#define FILE_NULL  (-1)

int file_close(file f)
{
	return close(f);
}

/** Get last system error code */
int err_last()
{
	return errno;
}

/** Get pointer to a system error message */
const char* err_strptr(int code)
{
	return strerror(code);
}

#endif

#define DIE(ex) \
	shout_and_die(ex, __FUNCTION__, __FILE__, __LINE__)

void shout_and_die(int ex, const char *func, const char *file, int line)
{
	if (!ex)
		return;

	// get error number from our last system call
	int e = err_last();

	// get error message for our error number
	const char *err = err_strptr(e);

	// use C-library function to print the error message to stdout
	printf("fatal error in %s(), %s:%d: (%d) %s\n"
		, func, file, line, e, err);

	// terminate the process
	_exit(1);
}

void main()
{
	// try to close the invalid file descriptor
	int r = file_close(FILE_NULL);
	DIE(r != 0);
}
