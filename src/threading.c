#include <stdlib.h>
#include "newsraft.h"

struct responsive_thread {
	pthread_t thread;
	bool was_started;
	volatile bool says_it_is_done;
};

static struct responsive_thread *threads = NULL;
static size_t threads_count;
static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
static const struct timespec update_routine_finish_check_period = {0, 10000000};

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

void
branch_update_feed_action_into_thread(void *(*action)(void *arg), struct feed_entry *feed)
{
	pthread_mutex_lock(&threads_lock);
	while (true) {
		for (size_t i = 0; i < threads_count; ++i) {
			if (threads[i].was_started == false) {
				threads[i].was_started = true;
				threads[i].says_it_is_done = false;
				feed->did_update_just_finished = &threads[i].says_it_is_done;
				pthread_create(&(threads[i].thread), NULL, action, feed);
				pthread_mutex_unlock(&threads_lock);
				return;
			}
		}
		// Note to the future.
		// We add a little delay before checking for finished threads so that
		// the CPU is not overloaded by running through this loop very fast.
		nanosleep(&update_routine_finish_check_period, NULL);
		for (size_t i = 0; i < threads_count; ++i) {
			if (threads[i].says_it_is_done == true) {
				pthread_join(threads[i].thread, NULL);
				threads[i].was_started = false;
				threads[i].says_it_is_done = false;
				break;
			}
		}
	}
	pthread_mutex_unlock(&threads_lock);
}

void
wait_for_all_threads_to_finish(bool block_threading)
{
	pthread_mutex_lock(&threads_lock);
	for (size_t i = 0; i < threads_count; ++i) {
		if (threads[i].was_started == true) {
			pthread_join(threads[i].thread, NULL);
			threads[i].was_started = false;
			threads[i].says_it_is_done = false;
		}
	}
	if (block_threading == true) {
		threads_count = 0;
	}
	pthread_mutex_unlock(&threads_lock);
}

void
terminate_update_threads(void)
{
	free(threads);
}
