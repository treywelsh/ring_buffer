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
 * Thread safety note :
 * The sequential consistency model ensure threads synchronization via the
 * C11 atomics.
 * "whenever thread T2 sees a modification thread T1 has effected on an"
 * atomic variable A, all side effects before that modication in thread T1
 * are visible to T2."
 * "The functional writing of atomic operations (e.g atomic_load)
 * is superfluous for sequential consistency"
 *
 */

#define ringb_get_len(ring) ((ring)->bufmask + 1)
#define ringb_is_full(ring) ((ring)->r_i == (((ring)->w_i + 1) & \
	(ring)->bufmask))
#define ringb_is_empty(ring) ((ring)->w_i == (ring)->r_i)

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
	r->w_i = 0;
	r->r_i = 0;
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
#define ringb_incr_r_i(ring) ((ring)->r_i = ((ring)->r_i + 1) & (ring)->bufmask)

/* Don't add to a full buffer */
#define ringb_add_unsafe(ring, elt) do { \
		(ring)->buf[(ring)->w_i] = (elt); \
		(ring)->w_i = ((ring)->w_i + 1) & (ring)->bufmask; \
} while (0)

/* Don't get from an empty buffer */
#define ringb_get_unsafe(ring, elt) do { \
		ringb_get_last_unsafe(ring, elt); \
		ringb_incr_r_i(ring); \
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
