#ifndef RINGB_H_
#define RINGB_H_

#include <assert.h>
#include <stdlib.h>

#if defined(RINGB_SPSC_SAFE) && defined(__STDC_NO_ATOMICS__)
#error "There is no stdatomic header."
#endif

#include <stdatomic.h>

/* Type of data stored */
typedef unsigned int ringb_data_t;

struct ringb {
	ringb_data_t *buf;
	unsigned int bufmask;

#ifdef RINGB_SPSC_SAFE
	atomic_uint w_i, r_i;
#else
	unsigned int w_i, r_i;
#endif
};
typedef struct ringb ringb_t;

/*
 * The ring buffer is full when : read_index == (write index + 1) % mask
 * Because of this choice (Which has been made in order to distinguish
 * full and empty cases), the real size is decremented of one element.
 *
 */

#define ringb_get_len(ring) ((ring)->bufmask + 1)

#ifdef RINGB_SPSC_SAFE

#define ringb_is_full(ring) (atomic_load_explicit(&(ring)->r_i, \
	memory_order_acquire) == \
	(((ring)->w_i + 1) & (ring)->bufmask))
#define ringb_is_empty(ring) (atomic_load_explicit(&(ring)->w_i, \
	memory_order_acquire) == \
	(ring)->r_i)

#else

#define ringb_is_full(ring) ((ring)->r_i == (((ring)->w_i + 1) & \
	(ring)->bufmask))
#define ringb_is_empty(ring) ((ring)->w_i == (ring)->r_i)

#endif


static inline int
ringb_init(ringb_t *r, size_t r_two_pow_sz)
{
	size_t size;

	assert(r != NULL);

	size = 1 << r_two_pow_sz;

	r->buf = malloc(size * sizeof(*(r->buf)));
	if (r->buf == NULL) {
		return 1;
	}
#ifdef RINGB_SPSC_SAFE
    atomic_init(&r->w_i, 0);
    atomic_init(&r->r_i, 0);
#else
	r->w_i = 0;
	r->r_i = 0;
#endif
	r->bufmask = size - 1;

	return 0;
}

static inline void
ringb_clean(ringb_t *r)
{
	assert(r != NULL);

	free(r->buf);
}

/*
 * XXX Unsafe macros
 * These macros allow you to add/get elements without
 * knowing the ring state (empty/full).
 * A misuse of these operation may corrupt the data structure.
 *
 */

#define ringb_get_last_unsafe(ring, elt) (*(elt) = ((ring)->buf)[(ring)->r_i])

#ifdef RINGB_SPSC_SAFE
#define ringb_incr_idx(idx, mask) atomic_store_explicit(&(idx), (idx + 1) & \
	(mask), memory_order_release)
#else
#define ringb_incr_idx(idx, mask) ((idx) = (idx + 1) & (mask))
#endif

#define ringb_incr_r_i(ring) (ringb_incr_idx((ring)->r_i, (ring)->bufmask))
#define ringb_incr_w_i(ring) (ringb_incr_idx((ring)->w_i, (ring)->bufmask))

/* Don't add to a full buffer */
#define ringb_add_unsafe(ring, elt) do { \
		(ring)->buf[(ring)->w_i] = (elt); \
		ringb_incr_w_i((ring)); \
} while (0)

/* Don't get from an empty buffer */
#define ringb_get_unsafe(ring, elt) do { \
		ringb_get_last_unsafe(ring, elt); \
		ringb_incr_r_i((ring)); \
} while (0)


/*
 * XXX Safe functions
 */

/* If full, discard oldest element */
static inline void
ringb_force_add(ringb_t *ring, ringb_data_t elt)
{
	assert(ring != NULL);
	if (ringb_is_full(ring)) {
		ringb_incr_r_i(ring);
	}
	ringb_add_unsafe(ring, elt);
}

/* If full, do not add */
static inline int
ringb_add(ringb_t *ring, ringb_data_t elt)
{
	assert(ring != NULL);
	if (ringb_is_full(ring)) {
		return 1;
	}
	ringb_add_unsafe(ring, elt);
	return 0;
}

static inline int
ringb_get(ringb_t *ring, ringb_data_t *elt)
{
	assert(ring != NULL);
	assert(elt != NULL);

	if (ringb_is_empty(ring)) {
		return 1;
	}

	ringb_get_unsafe(ring, elt);
	return 0;
}

#endif /* RINGB_H_ */
