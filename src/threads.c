#include "newsraft.h"

struct newsraft_thread {
	pthread_t thread;
	void *(*worker)(void *);
};

static struct newsraft_thread newsraft_threads[] = {
	[NEWSRAFT_THREAD_DOWNLOAD] = {0, &downloader_worker},
	[NEWSRAFT_THREAD_SHRUNNER] = {0, &executor_worker},
	[NEWSRAFT_THREAD_DBWRITER] = {0, &inserter_worker},
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
	pthread_kill(newsraft_threads[thread_id].thread, SIGUSR1);
}

void
threads_stop(void)
{
	for (size_t i = 0; i < LENGTH(newsraft_threads); ++i) {
		threads_wake_up(i);
		pthread_join(newsraft_threads[i].thread, NULL);
	}
}
