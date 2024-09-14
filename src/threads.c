#include "newsraft.h"

struct newsraft_thread {
	pthread_t thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	void *(*worker)(void *);
};

static struct newsraft_thread newsraft_threads[] = {
	[NEWSRAFT_THREAD_DOWNLOAD] = {0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, &downloader_worker},
	[NEWSRAFT_THREAD_SHRUNNER] = {0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, &executor_worker},
	[NEWSRAFT_THREAD_DBWRITER] = {0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, &inserter_worker},
};

bool
threads_start(void)
{
	for (size_t i = 0; i < LENGTH(newsraft_threads); ++i) {
		if (pthread_create(&newsraft_threads[i].thread, NULL, newsraft_threads[i].worker, NULL) != 0) {
			write_error("Failed to start a thread!\n");
			return false;
		}
	}
	return true;
}

void
threads_wake_up(int thread_id)
{
	pthread_cond_signal(&newsraft_threads[thread_id].cond);
}

void
threads_take_a_nap(int thread_id)
{
	pthread_mutex_lock(&newsraft_threads[thread_id].mutex);
	struct timespec wake_up_time = {0};
	clock_gettime(CLOCK_REALTIME, &wake_up_time);
	wake_up_time.tv_sec += 1;
	pthread_cond_timedwait(
		&newsraft_threads[thread_id].cond,
		&newsraft_threads[thread_id].mutex,
		&wake_up_time
	);
	pthread_mutex_unlock(&newsraft_threads[thread_id].mutex);
}

void
threads_stop(void)
{
	for (size_t i = 0; i < LENGTH(newsraft_threads); ++i) {
		threads_wake_up(i);
		pthread_join(newsraft_threads[i].thread, NULL);
		pthread_cond_destroy(&newsraft_threads[i].cond);
	}
}
