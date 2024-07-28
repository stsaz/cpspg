/* Cross-Platform System Programming Guide: L2: inter-process named semaphore */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE cpsem;
#define CPSEM_NULL NULL
#define CPSEM_CREATE  1

cpsem cpsem_open(const char *name, unsigned int flags, unsigned int value)
{
	wchar_t wname[1000];
	if (0 == MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 1000))
		return NULL;

	if (flags == CPSEM_CREATE)
		return CreateSemaphoreW(NULL, value, 0xffff, wname);
	else if (flags == 0)
		return OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, 0, wname);

	SetLastError(ERROR_INVALID_PARAMETER);
	return NULL;
}

void cpsem_close(cpsem sem)
{
	CloseHandle(sem);
}

int cpsem_wait(cpsem sem)
{
	int r = WaitForSingleObject(sem, -1);
	if (r == WAIT_OBJECT_0)
		r = 0;
	return r;
}

int cpsem_unlink(const char *name)
{
	(void)name;
	return 0;
}

int cpsem_post(cpsem sem)
{
	return !ReleaseSemaphore(sem, 1, NULL);
}

ssize_t stdin_read(void *buf, size_t cap)
{
	DWORD r;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (GetConsoleMode(h, &r)) {
		wchar_t w[1000];
		if (!ReadConsoleW(h, w, 1000, &r, NULL))
			return -1;

		return WideCharToMultiByte(CP_UTF8, 0, w, r, buf, cap, NULL, NULL);
	}

	if (!ReadFile(h, buf, cap, &r, 0))
		return -1;
	return r;
}

#else // UNIX:

#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

typedef sem_t* cpsem;
#define CPSEM_NULL  SEM_FAILED
#define CPSEM_CREATE  O_CREAT

/** Open or create a semaphore
flags: 0 or CPSEM_CREATE
value: initial value
Return CPSEM_NULL on error */
cpsem cpsem_open(const char *name, unsigned int flags, unsigned int value)
{
	return sem_open(name, flags, 0666, value);
}

/** Close semaphore */
void cpsem_close(cpsem sem)
{
	sem_close(sem);
}

/** Delete semaphore */
int cpsem_unlink(const char *name)
{
	return sem_unlink(name);
}

/** Decrease semaphore value, blocking if necessary */
int cpsem_wait(cpsem sem)
{
	return sem_wait(sem);
}

/** Increase semaphore value */
int cpsem_post(cpsem sem)
{
	return sem_post(sem);
}

ssize_t stdin_read(void *buf, size_t cap)
{
	return read(0, buf, cap);
}

#endif

void main(int argc, char **argv)
{
	const char *name = "/cpspg.sem";

	// unregister semaphore by user's command
	if (argc > 1 && !strcmp(argv[1], "unlink")) {
		assert(0 == cpsem_unlink(name));
		return;
	}

	// create new named semaphore with initial count = 1 or open an existing semaphore
	cpsem s = cpsem_open(name, CPSEM_CREATE, 1);
	assert(s != CPSEM_NULL);

	// try to decrease the counter to enter the protected region
	assert(0 == cpsem_wait(s));
	puts("Entered semaphore-protected region.  Press Enter to exit");

	char buf[1];
	stdin_read(buf, sizeof(buf));

	// increase the counter on leaving the protected region
	assert(0 == cpsem_post(s));

	cpsem_close(s);
}
