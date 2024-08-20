/* Cross-Platform System Programming Guide: L1: unnamed pipe I/O */

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE pipe_t;
#define FFPIPE_NULL  INVALID_HANDLE_VALUE

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

ssize_t pipe_write(pipe_t p, const void *data, size_t size)
{
	DWORD wr;
	if (!WriteFile(p, data, size, &wr, 0))
		return -1;
	return wr;
}

#else // UNIX:

#include <unistd.h>

typedef int pipe_t;
#define FFPIPE_NULL  (-1)

/** Create an unnamed pipe.
Return 0 on success */
int pipe_create(pipe_t *rd, pipe_t *wr)
{
	pipe_t p[2];
	if (0 != pipe(p))
		return -1;
	*rd = p[0];
	*wr = p[1];
	return 0;
}

/** Close a pipe */
void pipe_close(pipe_t p)
{
	close(p);
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

void main()
{
	// create a pipe
	pipe_t r, w;
	assert(0 == pipe_create(&r, &w));

	// write data to pipe
	ssize_t n = pipe_write(w, "hello!", 6);
	assert(n >= 0);

	// close writing descriptor
	pipe_close(w);

	// read data from pipe
	char buf[100];
	n = pipe_read(r, buf, sizeof(buf));
	assert(n >= 0);

	n = pipe_read(r, buf, sizeof(buf));
	assert(n == 0);

	// close reading descriptor
	pipe_close(r);
}
