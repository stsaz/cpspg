/* Cross-Platform System Programming Guide: L1: get/set file properties */

#include <assert.h>

/** Timestamp object structure with seconds since year 1 */
typedef struct {
	long long sec;
	unsigned int nsec;
} datetime;

/** Seconds passed before 1970 */
#define TIME_1970_SECONDS  62135596800ULL

#ifdef _WIN32

#include <windows.h>

typedef HANDLE file;
#define FILE_NULL  INVALID_HANDLE_VALUE
#define FILE_CREATENEW  CREATE_NEW
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

typedef BY_HANDLE_FILE_INFORMATION fileinfo;

int file_info(file f, fileinfo *fi)
{
	return !GetFileInformationByHandle(f, fi);
}

unsigned long long fileinfo_size(const fileinfo *fi)
{
	return ((unsigned long long)fi->nFileSizeHigh << 32) | fi->nFileSizeLow;
}

#define TIME_100NS  116444736000000000ULL // 100-ns intervals within 1600..1970

datetime datetime_from_filetime(FILETIME ft)
{
	datetime t = {};
	unsigned long long i = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	if (i > TIME_100NS) {
		i -= TIME_100NS;
		t.sec = TIME_1970_SECONDS + i / (1000000 * 10);
		t.nsec = (i % (1000000 * 10)) * 100;
	}
	return t;
}

FILETIME datetime_to_filetime(datetime t)
{
	t.sec -= TIME_1970_SECONDS;
	unsigned long long d = t.sec * 1000000 * 10 + t.nsec / 100 + TIME_100NS;
	FILETIME ft = {
		.dwLowDateTime = (unsigned int)d,
		.dwHighDateTime = (unsigned int)(d >> 32),
	};
	return ft;
}

datetime fileinfo_mtime(const fileinfo *fi)
{
	return datetime_from_filetime(fi->ftLastWriteTime);
}

unsigned int fileinfo_attr(const fileinfo *fi)
{
	return fi->dwFileAttributes;
}

int file_isdir(unsigned int file_attr)
{
	return ((file_attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

int file_set_mtime(file f, datetime last_write)
{
	FILETIME ft = datetime_to_filetime(last_write);
	return !SetFileTime(f, NULL, &ft, &ft);
}

int file_set_attr(file f, unsigned int attr)
{
	FILE_BASIC_INFO i = {};
	i.FileAttributes = attr;
	return !SetFileInformationByHandle(f, FileBasicInfo, &i, sizeof(FILE_BASIC_INFO));
}

#else // UNIX:

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

typedef int file;
#define FILE_NULL  (-1)
#define FILE_CREATENEW  (O_CREAT | O_EXCL)
#define FILE_WRITE  O_WRONLY

file file_open(const char *name, unsigned int flags)
{
	return open(name, flags, 0666);
}

int file_close(file f)
{
	return close(f);
}

typedef struct stat fileinfo;

/** Get file status by file descriptor.
Return 0 on success */
int file_info(file f, fileinfo *fi)
{
	return fstat(f, fi);
}

/** Get file size from fileinfo */
unsigned long long fileinfo_size(const fileinfo *fi)
{
	return fi->st_size;
}

/** Convert timespec -> datetime */
datetime datetime_from_timespec(struct timespec ts)
{
	datetime t = {
		.sec = TIME_1970_SECONDS + ts.tv_sec,
		.nsec = (unsigned int)ts.tv_nsec,
	};
	return t;
}

/** Convert datetime -> timeval */
struct timeval datetime_to_timeval(datetime t)
{
	struct timeval tv = {
		.tv_sec = t.sec - TIME_1970_SECONDS,
		.tv_usec = t.nsec / 1000,
	};
	return tv;
}

/** Get last-write time from fileinfo */
datetime fileinfo_mtime(const fileinfo *fi)
{
#if defined __APPLE__ && defined __MACH__
	return datetime_from_timespec(fi->st_mtimespec);
#else
	return datetime_from_timespec(fi->st_mtim);
#endif
}

/** Get file attributes from fileinfo.
Return OS-specific value */
unsigned int fileinfo_attr(const fileinfo *fi)
{
	return fi->st_mode;
}

/** Check whether directory flag is set in file attributes */
int file_isdir(unsigned int file_attr)
{
	return ((file_attr & S_IFMT) == S_IFDIR);
}

/** Set file last-modification time by descriptor.
Return 0 on success */
int file_set_mtime(file f, datetime last_write)
{
	struct timeval tv[2];
	tv[0] = datetime_to_timeval(last_write);
	tv[1] = datetime_to_timeval(last_write);
	return futimes(f, tv);
}

/** Set file attributes.
Return 0 on success */
int file_set_attr(file f, unsigned int mode)
{
	return fchmod(f, mode);
}

#endif

void main()
{
	// create a new file
	file f = file_open("file-props.tmp", FILE_CREATENEW | FILE_WRITE);
	assert(f != FILE_NULL);

	// get file properties: size, modification time, attributes
	fileinfo fi = {};
	assert(0 == file_info(f, &fi));

	// file size == 0
	assert(0 == fileinfo_size(&fi));

	datetime t = fileinfo_mtime(&fi);

	// check if file is not a directory
	unsigned attr = fileinfo_attr(&fi);
	assert(!file_isdir(attr));

	// set mtime
	assert(0 == file_set_mtime(f, t));

	// set attributes
#ifdef _WIN32
	attr |= FILE_ATTRIBUTE_READONLY;
#else
	attr = 0600;
#endif
	assert(0 == file_set_attr(f, attr));

	file_close(f);
}
