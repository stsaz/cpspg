/* Cross-Platform System Programming Guide: L1: execute new program and read its output */

#include <assert.h>

#ifdef _WIN32

#include <windows.h>
typedef HANDLE file;

#else // UNIX:

typedef int file;

#endif

typedef struct {
	const char **argv;
	file in, out, err;
} ps_execinfo;

#ifdef _WIN32

ssize_t stdout_write(const void *data, size_t len)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD r;
	if (GetConsoleMode(h, &r)) {

		wchar_t w[1000];
		r = MultiByteToWideChar(CP_UTF8, 0, data, len, w, 1000);

		if (!WriteConsoleW(h, w, r, &r, NULL))
			return -1;
		return len;
	}

	if (!WriteFile(h, data, len, &r, 0))
		return -1;
	return r;
}

typedef HANDLE pipe_t;
#define PIPE_NULL  INVALID_HANDLE_VALUE

int pipe_create(pipe_t *rd, pipe_t *wr)
{
	return !CreatePipe(rd, wr, NULL, 0);
}

void pipe_close(pipe_t p)
{
	CloseHandle(p);
}

ssize_t pipe_read(pipe_t p, void *buf, size_t cap)
{
	DWORD rd;
	if (!ReadFile(p, buf, cap, &rd, 0)) {
		if (GetLastError() == ERROR_BROKEN_PIPE)
			return 0;
		return -1;
	}
	return rd;
}

typedef HANDLE ps;
#define _PS_NULL  INVALID_HANDLE_VALUE

ps ps_exec_info(const char *filename, ps_execinfo *ei)
{
	wchar_t wfn[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfn, 1000))
		return _PS_NULL;

	// convert command-line arguments array to a single UTF-16 string
	wchar_t wargs[1000];
	size_t n = 0;
	for (size_t i = 0;  ei->argv[i] != NULL;  i++) {
		if (i != 0)
			wargs[n++] = ' ';
		int r = MultiByteToWideChar(CP_UTF8, 0, ei->argv[i], -1, &wargs[n], 1000 - n);
		if (r == 0)
			return _PS_NULL;
		r--;
		n += r;
	}

	STARTUPINFOW si = {};
	si.cb = sizeof(STARTUPINFO);
	if (ei->in != INVALID_HANDLE_VALUE || ei->out != INVALID_HANDLE_VALUE || ei->err != INVALID_HANDLE_VALUE) {
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.dwFlags |= STARTF_USESTDHANDLES;

		if (ei->in != INVALID_HANDLE_VALUE) {
			si.hStdInput = ei->in;
			SetHandleInformation(ei->in, HANDLE_FLAG_INHERIT, 1);
		}
		if (ei->out != INVALID_HANDLE_VALUE) {
			si.hStdOutput = ei->out;
			SetHandleInformation(ei->out, HANDLE_FLAG_INHERIT, 1);
		}
		if (ei->err != INVALID_HANDLE_VALUE) {
			si.hStdError = ei->err;
			SetHandleInformation(ei->err, HANDLE_FLAG_INHERIT, 1);
		}
	}

	PROCESS_INFORMATION info;
	if (!CreateProcessW(wfn, wargs, NULL, NULL, /*inherit handles*/ 1
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

ssize_t stdout_write(const void *data, size_t len)
{
	return write(1, data, len);
}

typedef int pipe_t;
#define PIPE_NULL  (-1)

int pipe_create(pipe_t *rd, pipe_t *wr)
{
	pipe_t p[2];
	if (0 != pipe(p))
		return -1;
	*rd = p[0];
	*wr = p[1];
	return 0;
}

void pipe_close(pipe_t p)
{
	close(p);
}

ssize_t pipe_read(pipe_t p, void *buf, size_t size)
{
	return read(p, buf, size);
}

extern char **environ;

typedef int ps;
#define _PS_NULL  (-1)

/** Create a new process.
Return process descriptor;
  _PS_NULL on error */
ps ps_exec_info(const char *filename, ps_execinfo *info)
{
	pid_t p = vfork();
	if (p != 0)
		return p;

	if (info->in != -1)
		dup2(info->in, STDIN_FILENO);
	if (info->out != -1)
		dup2(info->out, STDOUT_FILENO);
	if (info->err != -1)
		dup2(info->err, STDERR_FILENO);

	execve(filename, (char**)info->argv, environ);
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
	const char *path = "std-echo", *arg0 = "std-echo";
#ifdef _WIN32
	path = "std-echo.exe";
#endif

	// create a pipe which will act as a bridge between our process and the child process
	pipe_t r, w;
	assert(0 == pipe_create(&r, &w));

	// create a new process which will use our pipe for stdout/stderr
	const char *args[] = {
		arg0,
		NULL,
	};
	ps_execinfo info = {
		args,
		.in = PIPE_NULL,
		.out = w,
		.err = w,
	};
	ps p = ps_exec_info(path, &info);
	assert(p != _PS_NULL);

	// read the child's output data
	char buf[1000];
	ssize_t n = pipe_read(r, buf, sizeof(buf));
	assert(n >= 0);

	// print the data to stdout
	stdout_write(buf, n);

	ps_close(p);
	pipe_close(r);
	pipe_close(w);
}
