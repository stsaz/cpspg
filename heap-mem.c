/* Cross-Platform System Programming Guide: L1: heap memory allocation */

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

void* heap_alloc(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void heap_free(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

#else // UNIX:

#include <stdlib.h>

/** Allocate heap memory region.
Return buffer pointer or NULL on error */
void* heap_alloc(size_t size)
{
	return malloc(size);
}

/** Free heap memory region */
void heap_free(void *ptr)
{
	free(ptr);
}

#endif

void main()
{
	// allocate 8MB buffer on the heap
	char *buf = heap_alloc(8*1024*1024);
	assert(buf != NULL);

	// use the buffer
	buf[0] = '#';

	// free the allocated buffer
	heap_free(buf);
}
