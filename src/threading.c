#include <stdlib.h>
#include "newsraft.h"

struct responsive_thread {
	pthread_t thread;
	bool was_started;
};

static struct responsive_thread *threads = NULL;
static size_t threads_count;

bool
initialize_update_threads(void)
{
	threads_count = get_cfg_uint(CFG_UPDATE_THREADS_COUNT);
	if (threads_count != 0) {
		threads = calloc(threads_count, sizeof(struct responsive_thread));
		if (threads == NULL) {
			return false;
		}
	}
	return true;
}

static inline void
wait_for_any_thread_to_finish(void)
{
	for (size_t i = 0; i < threads_count; ++i) {
		if (threads[i].was_started == true) {
			pthread_join(threads[i].thread, NULL);
			threads[i].was_started = false;
			return;
		}
	}
}

static inline bool
start_any_thread(void *(*action)(void *arg), struct feed_line *feed)
{
	for (size_t i = 0; i < threads_count; ++i) {
		if (threads[i].was_started == false) {
			threads[i].was_started = true;
			pthread_create(&(threads[i].thread), NULL, action, feed);
			return true;
		}
	}
	return false;
}

void
branch_update_feed_action_into_thread(void *(*action)(void *arg), struct feed_line *feed)
{
	if (threads_count == 0) {
		action(feed);
	} else {
		while (start_any_thread(action, feed) == false) {
			wait_for_any_thread_to_finish();
		}
	}
}

void
wait_for_all_threads_to_finish(void)
{
	for (size_t i = 0; i < threads_count; ++i) {
		if (threads[i].was_started == true) {
			pthread_join(threads[i].thread, NULL);
			threads[i].was_started = false;
		}
	}
}

void
terminate_update_threads(void)
{
	free(threads);
}
