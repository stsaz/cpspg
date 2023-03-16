/* Cross-Platform System Programming Guide: L1: execute a new program */

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE ps;
#define _PS_NULL  INVALID_HANDLE_VALUE

ps ps_exec(const char *filename, const char **argv)
{
	wchar_t wfn[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfn, 1000))
		return _PS_NULL;

	// convert command-line arguments array to a single UTF-16 string
	wchar_t wargs[1000];
	size_t n = 0;
	for (size_t i = 0;  argv[i] != NULL;  i++) {
		if (i != 0)
			wargs[n++] = ' ';
		int r = MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, &wargs[n], 1000 - n);
		if (r == 0)
			return _PS_NULL;
		r--;
		n += r;
	}

	STARTUPINFOW si = {};
	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION info;
	if (!CreateProcessW(wfn, wargs, NULL, NULL, /*inherit handles*/ 0
		, 0, /*environment*/ NULL, NULL, &si, &info))
		return _PS_NULL;

	CloseHandle(info.hThread);
	return info.hProcess;
}

void ps_close(ps p)
{
	CloseHandle(p);
}

#else // UNIX:

#include <unistd.h>

extern char **environ;

typedef int ps;
#define _PS_NULL  (-1)

/** Create a new process.
Return process descriptor;
  _PS_NULL on error */
ps ps_exec(const char *filename, const char **argv)
{
	pid_t p = vfork();
	if (p != 0)
		return p;

	execve(filename, (char**)argv, environ);
	_exit(255);
	return 0;
}

/** Close process descriptor */
void ps_close(ps p)
{
	(void)p;
}

#endif

void main()
{
	const char *path = "dir-list", *arg0 = "dir-list";
#ifdef _WIN32
	path = "dir-list.exe";
#endif

	// create a new process
	const char *args[] = {
		arg0,
		NULL,
	};
	ps p = ps_exec(path, args);
	assert(p != _PS_NULL);

	// close process descriptor
	ps_close(p);
}
