#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#include "ringb.h"

#define THREAD_COUNT 4
#define THREAD_QUEUE_POW_SZ 3

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* pthrd_start argument */
struct thrd_info {
	pthread_t id;        /* ID returned by pthrd_create() */
	size_t num;          /* Application-defined thread # */
	atomic_bool alive;

	ringb_t queue;
};

static inline struct thrd_info *
thrds_init(size_t count, size_t queue_sz)
{
	size_t tnum;
	struct thrd_info *tinfo;

	/* Allocate memory for pthread_create() arguments */
	tinfo = calloc(count, sizeof(struct thrd_info));
	if (tinfo == NULL) {
		handle_error("calloc");
	}

	/* Init threads  */
	for (tnum = 0; tnum < count; tnum++) {
		tinfo[tnum].num = tnum + 1;
		ringb_init(&tinfo[tnum].queue, queue_sz);
	}

	return tinfo;
}

static void
thrds_kill(struct thrd_info *tinfo)
{
	size_t tnum;
	for (tnum = 0; tnum < THREAD_COUNT; tnum++) {
		tinfo[tnum].alive = false;
	}
}

static void
thrds_clean(struct thrd_info *tinfo)
{
	free(tinfo);
}

static void *
thrd_work(void *arg)
{
	struct thrd_info *tinfo = arg;
	unsigned int elt;

	atomic_init(&tinfo->alive, true);

	/* busy waiting when queue empty */
	while (tinfo->alive) {
		if (ringb_get(&tinfo->queue, &elt)) {
			sched_yield();
			continue;
		}
		printf("Thread %zu deq %d\n",
			 tinfo->num, elt);
	}

	pthread_exit(NULL);
}

/* TODO one log file per thread, joined into one at end */

int
main(void)
{
	int s;
	unsigned int tnum, i;
	struct thrd_info *tinfo;

	/* Init threads */
	tinfo = thrds_init(THREAD_COUNT, THREAD_QUEUE_POW_SZ);

	/* Fill queue before startup  */
	for (tnum = 0; tnum < THREAD_COUNT; tnum++) {
		for (i = 0; !ringb_is_full(&tinfo[tnum].queue); i++) {
			ringb_add(&tinfo[tnum].queue, i);
			printf("Master to thread %zu enq : %d\n"
				 , tinfo[tnum].num, i);
		}
		printf("Thread %zu queue full\n"
			 , tinfo[tnum].num);
	}
	fflush(stdout);

	/* Create threads */
	for (tnum = 0; tnum < THREAD_COUNT; tnum++) {

		s = pthread_create(&tinfo[tnum].id, NULL,
			&thrd_work, &tinfo[tnum]);
		if (s != 0) {
			handle_error_en(s, "pthrd_create");
		} else {
			printf("Thread %zu created\n"
				 , tinfo[tnum].num);
		}
	}

	/* Fill queue at runtime */
	for (i = ringb_get_len(&tinfo[tnum].queue) - 1; i < 20000; i++) {
		for (tnum = 0; tnum < THREAD_COUNT; tnum++) {
			while (ringb_add(&tinfo[tnum].queue, i)) {
				usleep(10);
				continue;
			}
			printf("Master to thread %zu enq : %d\n"
				 , tinfo[tnum].num, i);
		}
	}

	/* XXX wait thread end job */
	sleep(1);

	printf("Kill threads\n");
	thrds_kill(tinfo);

	/* join */
	for (tnum = 0; tnum < THREAD_COUNT; tnum++) {
		s = pthread_join(tinfo[tnum].id, NULL);
		if (s != 0)
			handle_error_en(s, "pthrd_join");

		printf("Joined with thread %zu\n"
			 , tinfo[tnum].num);
	}

	thrds_clean(tinfo);
	exit(EXIT_SUCCESS);
}
