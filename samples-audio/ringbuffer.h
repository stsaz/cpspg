/** Audio API Quick Start Guide: Ring buffer (for sample code only) */

#include <string.h>
#include <stdlib.h>

#define INT_READONCE(obj)  (*(volatile __typeof__(obj)*)&(obj))
#define INT_WRITEONCE(obj, val)  (*(volatile __typeof__(obj)*)&(obj) = (val))

#define fence_release()  __asm volatile("" : : : "memory")
#define fence_acquire()  __asm volatile("" : : : "memory")

typedef struct {
	size_t cap;
	size_t mask;
	size_t whead, wtail;
	size_t rhead, rtail;
	char data[0];
} ringbuffer;

typedef struct {
	char *ptr;
	size_t len;
} ringbuffer_chunk;

/** Allocate buffer
cap: max size; automatically aligned to the power of 2
Return NULL on error */
static inline ringbuffer* ringbuf_alloc(size_t cap)
{
	int one = __builtin_clz(cap - 1) + 1;
	cap = 1U << (64 - one + 1);
	ringbuffer *b = (ringbuffer*)malloc(sizeof(ringbuffer) + cap);
	if (b == NULL)
		return NULL;
	b->whead = b->wtail = 0;
	b->rhead = b->rtail = 0;
	b->cap = cap;
	b->mask = cap - 1;
	return b;
}

static inline void ringbuf_free(ringbuffer *b)
{
	free(b);
}

/** Reserve contiguous free space region with the maximum size of 'n' bytes.
free: (output) amount of free space after the operation
Return value for ringbuf_write_finish() */
static inline size_t ringbuf_write_begin(ringbuffer *b, size_t n, ringbuffer_chunk *dst, size_t *free)
{
	size_t wh, nwh;
	wh = b->whead;
	size_t _free = b->cap + INT_READONCE(b->rtail) - wh;
	fence_acquire();

	size_t i = wh & b->mask;
	if (n > _free)
		n = _free;
	if (i + n > b->cap)
		n = b->cap - i;

	nwh = wh + n;
	b->whead = nwh;

	dst->ptr = b->data + i;
	dst->len = n;

	if (free != NULL)
		*free = _free - n;
	return wh;
}

/** Commit data reserved by ringbuf_write_begin().
nwh: return value from ringbuf_write_begin() */
static inline void ringbuf_write_finish(ringbuffer *b, size_t nwh)
{
	fence_release();
	INT_WRITEONCE(b->wtail, nwh);
}

/** Write some data
Return N of bytes written */
static inline size_t ringbuf_write(ringbuffer *b, const void *src, size_t n)
{
	ringbuffer_chunk d;
	size_t wh = ringbuf_write_begin(b, n, &d, NULL);
	if (d.len == 0)
		return 0;

	memcpy(d.ptr, src, d.len);
	ringbuf_write_finish(b, wh);
	return d.len;
}

/** Lock contiguous data region with the maximum size of 'n' bytes.
used: (output) amount of used space after the operation
Return value for ringbuf_read_finish() */
static inline size_t ringbuf_read_begin(ringbuffer *b, size_t n, ringbuffer_chunk *dst, size_t *used)
{
	size_t rh, nrh;
	rh = b->rhead;
	size_t _used = INT_READONCE(b->wtail) - rh;
	fence_acquire();

	size_t i = rh & b->mask;
	if (n > _used)
		n = _used;
	if (i + n > b->cap)
		n = b->cap - i;

	nrh = rh + n;
	b->rhead = nrh;

	dst->ptr = b->data + i;
	dst->len = n;

	if (used != NULL)
		*used = _used - n;
	return rh;
}

/** Discard the locked data region.
nrh: return value from ringbuf_read_begin() */
static inline void ringbuf_read_finish(ringbuffer *b, size_t nrh)
{
	fence_release();
	INT_WRITEONCE(b->rtail, nrh);
}
