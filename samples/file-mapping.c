/* Cross-Platform System Programming Guide: L2: inter-process file mapping
Usage:
	./file-mapping
						./file-mapping 'data from instance 2'
	<Enter>
	data from instance 2
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE file;
typedef HANDLE filemap;
#define FILE_NULL  INVALID_HANDLE_VALUE
#define _FILE_CREATE  OPEN_ALWAYS
#define FILE_READWRITE  (GENERIC_READ | GENERIC_WRITE)

file file_open(const char *name, unsigned int flags)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return FILE_NULL;

	unsigned int creation = flags & 0x0f;
	if (creation == 0)
		creation = OPEN_EXISTING;

	unsigned int access = flags & (GENERIC_READ | GENERIC_WRITE);
	unsigned int share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	return CreateFileW(w, access, share, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
}

int file_close(file f)
{
	return !CloseHandle(f);
}

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


#define FMAP_CREATE_READWRITE  PAGE_READWRITE

filemap fmap_create(file f, unsigned long long max_size, int access)
{
	return CreateFileMapping(f, NULL, access, (unsigned int)(max_size >> 32), (unsigned int)max_size, NULL);
}

void fmap_close(filemap fmap)
{
	CloseHandle(fmap);
}

#define FMAP_READWRITE  (FILE_MAP_READ | FILE_MAP_WRITE)
#define FMAP_SHARED  0

void* fmap_map(filemap fmap, unsigned long long offset, size_t size, int prot, int flags)
{
	return MapViewOfFile(fmap, prot, (unsigned int)(offset >> 32), (unsigned int)offset, size);
}

int fmap_unmap(void *p, size_t sz)
{
	return 0 == UnmapViewOfFile(p);
}

#else // UNIX:

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int file;
typedef int filemap;
#define FILE_NULL  (-1)
#define _FILE_CREATE  O_CREAT
#define FILE_READWRITE  O_RDWR

file file_open(const char *name, unsigned int flags)
{
	return open(name, flags, 0666);
}

int file_close(file f)
{
	return close(f);
}

int file_trunc(file f, unsigned long long len)
{
	return ftruncate(f, len);
}


#define FMAP_CREATE_READWRITE  0

/** Create file mapping */
filemap fmap_create(file f, unsigned long long max_size, int access)
{
	return f;
}

/** Close a file mapping object */
void fmap_close(filemap fmap)
{
}

#define FMAP_READWRITE  (PROT_READ | PROT_WRITE)
#define FMAP_SHARED  MAP_SHARED

/** Map file region into memory
prot: FMAP_READWRITE
flags: FMAP_SHARED
Return NULL on error */
void* fmap_map(filemap f, unsigned long long offset, size_t size, int prot, int flags)
{
	void *h = mmap(NULL, size, prot, flags, f, offset);
	return (h != MAP_FAILED) ? h : NULL;
}

/** Unmap the previously mapped region */
int fmap_unmap(void *p, size_t sz)
{
	return munmap(p, sz);
}

#endif

void main(int argc, char **argv)
{
	// open/create file
	file f = file_open("fmap.txt", _FILE_CREATE | FILE_READWRITE);
	assert(f != FILE_NULL);
	assert(0 == file_trunc(f, 4096));

	// get file mapping object
	filemap fm = fmap_create(f, 4096, FMAP_CREATE_READWRITE);

	// map file region to memory
	char *m = fmap_map(fm, 0, 4096, FMAP_READWRITE, FMAP_SHARED);
	assert(m != NULL);

	// we may close file and file mapping objects
	fmap_close(fm);
	file_close(f);

	if (argc > 1) {
		// write new data
		assert(strlen(argv[1]) <= 4096);
		memcpy(m, argv[1], strlen(argv[1]));

	} else {
		// wait until the user presses Enter
		char buf[1];
#ifdef _WIN32
		DWORD r;
		ReadFile(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &r, 0);
#else
		read(STDIN_FILENO, buf, 1);
#endif

		// print the existing data inside file mapping
		printf("%s\n", m);
	}

	// unmap file region
	assert(0 == fmap_unmap(m, 4096));
}
