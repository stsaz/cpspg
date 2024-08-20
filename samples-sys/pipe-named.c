/* Cross-Platform System Programming Guide: L2: inter-process named pipe I/O */

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE pipe_t;
#define PIPE_NULL  INVALID_HANDLE_VALUE
#define PIPE_NAME_PREFIX  "\\\\.\\pipe\\"

pipe_t pipe_create_named(const char *name)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return INVALID_HANDLE_VALUE;

	return CreateNamedPipeW(w
		, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE
		, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
		, PIPE_UNLIMITED_INSTANCES, 512, 512, 0, NULL);
}

void pipe_close(pipe_t p)
{
	CloseHandle(p);
}

void pipe_peer_close(pipe_t p)
{
	DisconnectNamedPipe(p);
}

pipe_t pipe_accept(pipe_t listen_pipe)
{
	BOOL b = ConnectNamedPipe(listen_pipe, NULL);
	if (!b) {
		if (GetLastError() == ERROR_PIPE_CONNECTED)
			return listen_pipe; // the client has connected before we called ConnectNamedPipe()
		return INVALID_HANDLE_VALUE;
	}
	return listen_pipe;
}

pipe_t pipe_connect(const char *name)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return INVALID_HANDLE_VALUE;

	return CreateFileW(w, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
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

ssize_t pipe_write(pipe_t p, const void *data, size_t size)
{
	DWORD wr;
	if (!WriteFile(p, data, size, &wr, 0))
		return -1;
	return wr;
}

#else // UNIX:

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>

typedef int pipe_t;
#define PIPE_NULL  (-1)

/** Create named pipe
name:
  UNIX: file name to be used for UNIX socket
  Windows: \\.\pipe\NAME
Return PIPE_NULL on error */
pipe_t pipe_create_named(const char *name)
{
	pipe_t p;
	struct sockaddr_un a = {};
#if (!defined __linux__ && defined __unix__) || (defined __APPLE__ && defined __MACH__)
	a.sun_len = sizeof(struct sockaddr_un);
#endif
	a.sun_family = AF_UNIX;
	size_t len = strlen(name);
	if (len + 1 > sizeof(a.sun_path)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(a.sun_path, name, len + 1);

	if (-1 == (p = socket(AF_UNIX, SOCK_STREAM, 0)))
		return -1;

	if (0 != bind(p, (struct sockaddr*)&a, sizeof(struct sockaddr_un)))
		goto err;

	if (0 != listen(p, 0))
		goto err;

	return p;

err:
	close(p);
	return -1;
}

/** Close a pipe */
void pipe_close(pipe_t p)
{
	close(p);
}

/** Close a peer pipe descriptor returned by pipe_accept() */
void pipe_peer_close(pipe_t p)
{
	close(p);
}

/** Connect to a named pipe
Return PIPE_NULL on error */
pipe_t pipe_connect(const char *name)
{
	pipe_t p;
	struct sockaddr_un a = {};
#if (!defined __linux__ && defined __unix__) || (defined __APPLE__ && defined __MACH__)
	a.sun_len = sizeof(struct sockaddr_un);
#endif
	a.sun_family = AF_UNIX;
	size_t len = strlen(name);
	if (len + 1 > sizeof(a.sun_path)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(a.sun_path, name, len + 1);

	if (-1 == (p = socket(AF_UNIX, SOCK_STREAM, 0)))
		return -1;

	if (0 != connect(p, (struct sockaddr*)&a, sizeof(struct sockaddr_un))) {
		close(p);
		return -1;
	}

	return p;
}

/** Accept an inbound connection to a named pipe
Return pipe descriptor; close with pipe_peer_close()
    Windows: the same pipe fd is returned
    UNIX: new fd to a UNIX socket's peer is returned
  PIPE_NULL on error */
pipe_t pipe_accept(pipe_t listen_pipe)
{
	return accept(listen_pipe, NULL, NULL);
}

/** Read from a pipe descriptor.
Return N of bytes read;
  <0 on error */
ssize_t pipe_read(pipe_t p, void *buf, size_t size)
{
	return read(p, buf, size);
}

/** Write to a pipe descriptor.
Return N of bytes written;
  <0 on error */
ssize_t pipe_write(pipe_t p, const void *buf, size_t size)
{
	return write(p, buf, size);
}

#endif

void main(int argc, char **argv)
{
	const char *name = "/tmp/cpspg.pipe";
#ifdef _WIN32
	name = PIPE_NAME_PREFIX "cpspg.pipe";
#endif

	if (argc > 1 && !strcmp(argv[1], "server")) {

#ifndef _WIN32
		unlink(name);
#endif

		// create a pipe
		pipe_t p;
		assert(PIPE_NULL != (p = pipe_create_named(name)));

		// wait for incoming connection
		pipe_t pc;
		assert(PIPE_NULL != (pc = pipe_accept(p)));

		// read data
		char buf[100];
		ssize_t n = pipe_read(pc, buf, sizeof(buf));
		assert(n >= 0);

		printf("%.*s\n", (int)n, buf);

		pipe_peer_close(pc);
		pipe_close(p);
		return;
	}

	// connect to a pipe
	pipe_t p;
	assert(PIPE_NULL != (p = pipe_connect(name)));

	// write data
	ssize_t n = pipe_write(p, "hello!", 6);
	assert(n >= 0);

	pipe_close(p);
}
