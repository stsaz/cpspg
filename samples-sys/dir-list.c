/* Cross-Platform System Programming Guide: L1: print the current directory contents to console */

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

#define ERR_NOMOREFILES  ERROR_NO_MORE_FILES

typedef struct {
	HANDLE dir;
	WIN32_FIND_DATAW data; // Windows fills this object for us for each file found in the directory
	char name[260 * 4]; // UTF-8 buffer large enough to hold 'data.cFileName'
	unsigned next;
} dirscan;

int dirscan_open(dirscan *d, const char *path)
{
	if (path[0] == '\0') {
		SetLastError(ERROR_PATH_NOT_FOUND);
		return -1;
	}

	// Append "\\*" to the directory path
	wchar_t w[1000];
	int r = MultiByteToWideChar(CP_UTF8, 0, path, -1, w, 1000 - 2);
	if (r == 0)
		return -1;
	r--;
	w[r++] = '\\';
	w[r++] = '*';
	w[r] = '\0';

	// open directory and get its first file
	HANDLE dir = FindFirstFileW(w, &d->data);
	if (dir == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND)
		return -1;

	d->dir = dir;
	return 0;
}

void dirscan_close(dirscan *d)
{
	FindClose(d->dir);
	d->dir = INVALID_HANDLE_VALUE;
}

const char* dirscan_next(dirscan *d)
{
	if (!d->next) {
		// 1. We already have the info on the first file
		if (d->dir == INVALID_HANDLE_VALUE) {
			SetLastError(ERROR_NO_MORE_FILES);
			return NULL;
		}
		d->next = 1;

	} else {
		// 2. Get info on the next file in directory
		if (!FindNextFileW(d->dir, &d->data))
			return NULL;
	}

	if (0 == WideCharToMultiByte(CP_UTF8, 0, d->data.cFileName, -1, d->name, sizeof(d->name), NULL, NULL))
		return NULL;
	return d->name;
}

int err_last()
{
	return GetLastError();
}

#else // UNIX:

#include <dirent.h>
#include <errno.h>

#define ERR_NOMOREFILES  0

typedef struct {
	DIR *dir;
} dirscan;

/** Open directory listing.
Return 0 on success */
int dirscan_open(dirscan *d, const char *path)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
		return -1;
	d->dir = dir;
	return 0;
}

/** Close directory listing */
void dirscan_close(dirscan *d)
{
	closedir(d->dir);
	d->dir = NULL;
}

/** Get name (without path) of the next file in directory.
Return NULL if there are no more files left. */
const char* dirscan_next(dirscan *d)
{
	const struct dirent *de;
	errno = ERR_NOMOREFILES;
	if (NULL == (de = readdir(d->dir)))
		return NULL;
	return de->d_name;
}

int err_last()
{
	return errno;
}

#endif

void main()
{
	// open directory listing
	dirscan ds = {};
	assert(0 == dirscan_open(&ds, "."));

	// read file names one by one and print to stdout
	const char *name;
	while (NULL != (name = dirscan_next(&ds))) {
		puts(name);
	}
	assert(err_last() == ERR_NOMOREFILES);

	// close descriptor
	dirscan_close(&ds);
}
