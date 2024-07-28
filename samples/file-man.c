/* Cross-Platform System Programming Guide: L1: create/rename/delete file or directory */

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE file;
#define FILE_NULL  INVALID_HANDLE_VALUE
#define _FILE_CREATE  OPEN_ALWAYS
#define FILE_WRITE  GENERIC_WRITE

file file_open(const char *name, unsigned int flags)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return INVALID_HANDLE_VALUE;

	unsigned int creation = flags & 0x0f;
	if (creation == 0)
		creation = OPEN_EXISTING;

	unsigned int access = flags & (GENERIC_READ | GENERIC_WRITE);
	return CreateFileW(w, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
}

int file_close(file f)
{
	return !CloseHandle(f);
}

int file_rename(const char *oldpath, const char *newpath)
{
	wchar_t w_old[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, oldpath, -1, w_old, 1000))
		return -1;

	wchar_t w_new[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, newpath, -1, w_new, 1000))
		return -1;

	return !MoveFileExW(w_old, w_new, MOVEFILE_REPLACE_EXISTING);
}

int file_remove(const char *name)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return -1;

	return !DeleteFileW(w);
}

int dir_make(const char *name)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return -1;

	return !CreateDirectoryW(w, NULL);
}

int dir_remove(const char *name)
{
	wchar_t w[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 1000))
		return -1;

	return !RemoveDirectoryW(w);
}

#else // UNIX:

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int file;
#define FILE_NULL  (-1)
#define _FILE_CREATE  O_CREAT
#define FILE_WRITE  O_WRONLY

file file_open(const char *name, unsigned int flags)
{
	return open(name, flags, 0666);
}

int file_close(file f)
{
	return close(f);
}

/** Change the name or location of a file.
Return 0 on success */
int file_rename(const char *oldpath, const char *newpath)
{
	return rename(oldpath, newpath);
}

/** Delete a name and possibly the file it refers to.
Return 0 on success */
int file_remove(const char *name)
{
	return unlink(name);
}

/** Create a new directory.
Return 0 on success */
int dir_make(const char *name)
{
	return mkdir(name, 0777);
}

/** Delete directory.
Return 0 on success */
int dir_remove(const char *name)
{
	return rmdir(name);
}

#endif

void main()
{
	// create a new directory
	int r = dir_make("file-man-dir");
	assert(r == 0);

	// create a new empty file inside our directory
	file f = file_open("file-man-dir/file.tmp", _FILE_CREATE | FILE_WRITE);
	assert(f != FILE_NULL);
	file_close(f);

	// rename our file
	r = file_rename("file-man-dir/file.tmp", "file-man-dir/newfile.tmp");
	assert(r == 0);

	// delete our file
	r = file_remove("file-man-dir/newfile.tmp");
	assert(r == 0);

	// delete our (now empty) directory
	r = dir_remove("file-man-dir");
	assert(r == 0);
}
