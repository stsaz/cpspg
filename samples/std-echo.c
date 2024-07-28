/* Cross-Platform System Programming Guide: L1: Standard I/O echo
Usage:
	$ ./std-echo
	[We type:] hello!
	[The program prints:] hello!
*/

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

ssize_t stdin_read(void *buf, size_t cap)
{
	DWORD r;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (GetConsoleMode(h, &r)) {
		// stdin is a console.  Read UTF-16 characters and convert them to UTF-8.

		wchar_t w[1000];
		if (!ReadConsoleW(h, w, 1000, &r, NULL))
			return -1;

		return WideCharToMultiByte(CP_UTF8, 0, w, r, buf, cap, NULL, NULL);
	}

	// stdin is a pipe.  Read UTF-8 data as-is.
	if (!ReadFile(h, buf, cap, &r, 0))
		return -1;
	return r;
}

ssize_t stdout_write(const void *data, size_t len)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD r;
	if (GetConsoleMode(h, &r)) {
		// stdout is a console.  Convert UTF-8 to UTF-16, then write UTF-16 characters to console.

		wchar_t w[1000];
		r = MultiByteToWideChar(CP_UTF8, 0, data, len, w, 1000);

		if (!WriteConsoleW(h, w, r, &r, NULL))
			return -1;
		return len;
	}

	// stdout is a pipe.  Write UTF-8 data as-is.
	if (!WriteFile(h, data, len, &r, 0))
		return -1;
	return r;
}

#else // UNIX:

#include <unistd.h>

/** Read from stdin.
Return N of bytes read;
  <0 on error */
ssize_t stdin_read(void *buf, size_t cap)
{
	return read(STDIN_FILENO, buf, cap);
}

/** Write to stdout.
Return N of bytes written;
  <0 on error */
ssize_t stdout_write(const void *data, size_t len)
{
	return write(STDOUT_FILENO, data, len);
}

#endif

void main()
{
	for (;;) {
		char buf[1000];
		// read some data from the user
		ssize_t r = stdin_read(buf, sizeof(buf));
		if (r <= 0)
			return;

		// write the data back to the user
		const char *d = buf;
		while (r != 0) {
			ssize_t w = stdout_write(d, r);
			assert(w >= 0);
			d += w;
			r -= w;
		}
	}
}
