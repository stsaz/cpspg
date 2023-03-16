/* Cross-Platform System Programming Guide: L1: copy/duplicate some data in file and truncate
Usage:
	$ echo hello! >file-echo.log
	$ ./file-echo-trunc
	$ cat file-echo.log
	lo!
*/

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE file;
#define FILE_NULL  INVALID_HANDLE_VALUE
#define FILE_READWRITE  (GENERIC_READ | GENERIC_WRITE)

file file_open(const char *name, unsigned int flags)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return FILE_NULL;

	unsigned int creation = OPEN_EXISTING;
	unsigned int access = flags & (GENERIC_READ | GENERIC_WRITE);
	return CreateFileW(w, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
}

int file_close(file f)
{
	return !CloseHandle(f);
}

ssize_t file_read(file f, void *buf, size_t cap)
{
	DWORD rd;
	if (!ReadFile(f, buf, cap, &rd, 0))
		return -1;
	return rd;
}

ssize_t file_write(file f, const void *data, size_t len)
{
	DWORD wr;
	if (!WriteFile(f, data, len, &wr, 0))
		return -1;
	return wr;
}

#define FILE_SEEK_BEGIN  FILE_BEGIN

long long file_seek(file f, unsigned long long pos, int method)
{
	long long r;
	if (!SetFilePointerEx(f, *(LARGE_INTEGER*)&pos, (LARGE_INTEGER*)&r, method))
		return -1;
	return r;
}

int file_trunc(file f, unsigned long long len)
{
	long long pos = file_seek(f, 0, FILE_CURRENT); // get current offset
	if (pos < 0)
		return -1;
	if (0 > file_seek(f, len, FILE_BEGIN)) // seek to the specified offset
		return -1;

	int r = !SetEndOfFile(f);

	if (0 > file_seek(f, pos, FILE_BEGIN)) // restore current offset
		r = -1;
	return r;
}

#else // UNIX:

#include <unistd.h>
#include <fcntl.h>

typedef int file;
#define FILE_NULL  (-1)
#define FILE_READWRITE  O_RDWR

/** Open or create a file.
Return FILE_NULL on error */
file file_open(const char *name, unsigned int flags)
{
	return open(name, flags, 0666);
}

/** Close a file descriptor.
Return !=0 on error, i.e. file isn't written completely */
int file_close(file f)
{
	return close(f);
}

/** Read data from a file descriptor.
Return N of bytes read;
  <0 on error */
ssize_t file_read(file f, void *buf, size_t cap)
{
	return read(f, buf, cap);
}

/** Write data to a file descriptor.
Return N of bytes written;
  <0 on error */
ssize_t file_write(file f, const void *data, size_t len)
{
	return write(f, data, len);
}

#define FILE_SEEK_BEGIN  SEEK_SET

long long file_seek(file f, unsigned long long pos, int method)
{
	return lseek(f, pos, method);
}

int file_trunc(file f, unsigned long long len)
{
	return ftruncate(f, len);
}

#endif

void main()
{
	// open the file for reading and writing
	file f = file_open("file-echo.log", FILE_READWRITE);
	assert(f != FILE_NULL);

	// read some data from file
	char buf[1000];
	ssize_t n = file_read(f, buf, sizeof(buf));
	assert(n >= 0);

	// set current offset to the beginning
	long long r = file_seek(f, 0, FILE_SEEK_BEGIN);
	assert(r >= 0);

	// write the data to file
	r = file_write(f, buf + n / 2, n - n / 2);
	assert(r >= 0);

	// truncate the file at our current offset
	assert(0 == file_trunc(f, r));

	// close the file
	file_close(f);
}
