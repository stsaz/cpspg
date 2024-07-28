/* Cross-Platform System Programming Guide: L2: execute a new program and wait for its termination */

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE ps;
#define _PS_NULL  INVALID_HANDLE_VALUE
#define ERR_TIMEDOUT  WSAETIMEDOUT

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

int ps_wait(ps p, int block, int *exit_code)
{
	unsigned int timeout_ms = (block) ? -1 : 0;
	int r = WaitForSingleObject(p, timeout_ms);
	if (r == WAIT_OBJECT_0) {
		if (exit_code != NULL)
			GetExitCodeProcess(p, (DWORD*)exit_code);
		CloseHandle(p);
		r = 0;

	} else if (r == WAIT_TIMEOUT) {
		SetLastError(WSAETIMEDOUT);
	}
	return r;
}

int ps_kill(ps p)
{
	return !TerminateProcess(p, -9);
}

unsigned int ps_id(ps p)
{
	return GetProcessId(p);
}

int err_last()
{
	return GetLastError();
}

#else // UNIX:

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

extern char **environ;

typedef int ps;
#define _PS_NULL  (-1)
#define ERR_TIMEDOUT  ETIMEDOUT

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

/** Wait for the process to exit
block:
  0: just check status (don't wait)
  !=0: wait forever
Return 0 on success.
  Process handle is closed, so don't call ps_close() in this case. */
int ps_wait(ps p, int block, int *exit_code)
{
	siginfo_t s = {};
	int f = (!block) ? WNOHANG : 0;
	int r = waitid(P_PID, (id_t)p, &s, WEXITED | f);
	if (r != 0)
		return -1;

	if (s.si_pid == 0) {
		errno = ETIMEDOUT;
		return -1;
	}

	if (exit_code != NULL) {
		if (s.si_code == CLD_EXITED)
			*exit_code = s.si_status;
		else
			*exit_code = -s.si_status;
	}
	return 0;
}

/** Kill the process */
int ps_kill(ps p)
{
	return kill(p, SIGKILL);
}

/** Get process ID by process descriptor */
unsigned int ps_id(ps p)
{
	return p;
}

int err_last()
{
	return errno;
}

#endif

void main()
{
	const char *path = "std-echo", *arg0 = "std-echo";
#ifdef _WIN32
	path = "std-echo.exe";
#endif

	// create a new process
	const char *args[] = {
		arg0,
		NULL,
	};
	ps p = ps_exec(path, args);
	assert(p != _PS_NULL);

	// get the child PID
	unsigned int pid = ps_id(p);
	printf("child PID: %u\n", pid);

	// check if the child is terminated already
	int code;
	assert(0 != ps_wait(p, 0, &code));
	assert(err_last() == ERR_TIMEDOUT);

	// forcefully terminate the child process
	assert(0 == ps_kill(p));

	// wait until process is terminated
	assert(0 == ps_wait(p, 1, &code));
	assert(code == -9);

	ps_close(p);
}
