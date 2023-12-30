#include "update_feed/update_feed.h"

struct responsive_thread {
	pthread_t thread;
	bool was_started;
	volatile bool says_it_is_done;
};

static struct feed_entry **update_queue = NULL;
static size_t update_queue_length = 0;
static size_t update_queue_progress = 0;
static size_t update_queue_failures = 0;
static size_t updates_finished = 0;
static size_t cumulative_new_items_count = 0;

static pthread_t queue_worker_thread;

static struct responsive_thread worker_threads[NEWSRAFT_THREADS_COUNT_LIMIT];
static size_t worker_threads_count;
static const struct timespec worker_threads_check_period = {0, 10000000}; // 0.01 seconds

static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;

static inline void
free_feed(struct getfeed_feed *feed)
{
	free_string(feed->title);
	free_string(feed->url);
	free_string(feed->content);
	free_string(feed->attachments);
	free_string(feed->persons);
	free_string(feed->extras);
	free_string(feed->http_header_etag);
	free_item(feed->item);
}

static void *
update_feed_action(void *arg)
{
	struct feed_entry *feed = arg;

	download_status status = DOWNLOAD_FAILED;
	struct stream_callback_data data = {0};

	data.feed.download_date = time(NULL);
	if (data.feed.download_date <= 0) {
		FAIL("Failed to obtain system time!");
		goto finish;
	}
	feed->download_date = data.feed.download_date;

	if (get_cfg_bool(CFG_RESPECT_EXPIRES_HEADER) == true) {
		int64_t expires_date = db_get_date_from_feeds_table(feed->link, "http_header_expires", 19);
		if (expires_date < 0) {
			goto finish;
		} else if ((expires_date > 0) && (data.feed.download_date < expires_date)) {
			INFO("Aborting update because content hasn't expired yet.");
			status = DOWNLOAD_CANCELED;
			goto finish;
		}
	}

	if (get_cfg_bool(CFG_RESPECT_TTL_ELEMENT) == true) {
		int64_t ttl = db_get_date_from_feeds_table(feed->link, "time_to_live", 12);
		int64_t prev_download_date = db_get_date_from_feeds_table(feed->link, "download_date", 13);
		if ((ttl < 0) || (prev_download_date < 0)) {
			goto finish;
		} else if ((ttl > 0) && (prev_download_date > 0) && ((prev_download_date + ttl) > data.feed.download_date)) {
			INFO("Aborting update because content hasn't died yet.");
			status = DOWNLOAD_CANCELED;
			goto finish;
		}
	}

	if (get_cfg_bool(CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) {
		data.feed.http_header_last_modified = db_get_date_from_feeds_table(feed->link, "http_header_last_modified", 25);
		if (data.feed.http_header_last_modified < 0) {
			goto finish;
		}
	}

	if (get_cfg_bool(CFG_SEND_IF_NONE_MATCH_HEADER) == true) {
		data.feed.http_header_etag = db_get_string_from_feed_table(feed->link, "http_header_etag", 16);
	}

	status = feed->link->ptr[0] == '$' ? execute_feed(feed->link, &data) : download_feed(feed->link->ptr, &data);

finish:
	pthread_mutex_lock(&update_lock);
	if (status == DOWNLOAD_SUCCEEDED) {
		if (insert_feed(feed, &data.feed) == false) {
			status = DOWNLOAD_FAILED;
		}
	}
	free_feed(&data.feed);
	if (status == DOWNLOAD_SUCCEEDED) {
		bool need_redraw = false;
		int64_t new_unread_count = get_unread_items_count_of_the_feed(feed->link);
		if (new_unread_count >= 0 && new_unread_count != feed->unread_count) {
			if (new_unread_count > feed->unread_count) {
				cumulative_new_items_count += new_unread_count - feed->unread_count;
			}
			feed->unread_count = new_unread_count;
			refresh_unread_items_count_of_all_sections();
			need_redraw = true;
		}
		if ((feed->name == NULL) || (feed->name->len == 0)) {
			struct string *title = db_get_string_from_feed_table(feed->link, "title", 5);
			if (title != NULL) {
				inlinefy_string(title);
				cpyss(&feed->name, title);
				free_string(title);
				need_redraw = true;
			}
		}
		if (need_redraw == true) {
			expose_all_visible_entries_of_the_list_menu();
		}
	} else if (status == DOWNLOAD_FAILED) {
		fail_status("Failed to update %s", feed->link->ptr);
		update_queue_failures += 1;
	} else if (status == DOWNLOAD_CANCELED) {
		INFO("Download canceled.");
		db_set_download_date(feed->link, feed->download_date);
	}
	updates_finished += 1;
	info_status("Feed updates completed: %zu/%zu", updates_finished, update_queue_length);
	pthread_mutex_unlock(&update_lock);
	*feed->did_update_just_finished = true;
	return NULL;
}

static void
branch_update_feed_action_into_thread(struct feed_entry *feed)
{
	while (they_want_us_to_terminate == false) {
		for (size_t i = 0; i < worker_threads_count; ++i) {
			if (worker_threads[i].was_started == false) {
				worker_threads[i].was_started = true;
				worker_threads[i].says_it_is_done = false;
				feed->did_update_just_finished = &worker_threads[i].says_it_is_done;
				pthread_create(&(worker_threads[i].thread), NULL, &update_feed_action, feed);
				return;
			} else if (worker_threads[i].says_it_is_done == true) {
				pthread_join(worker_threads[i].thread, NULL);
				worker_threads[i].was_started = true;
				worker_threads[i].says_it_is_done = false;
				feed->did_update_just_finished = &worker_threads[i].says_it_is_done;
				pthread_create(&(worker_threads[i].thread), NULL, &update_feed_action, feed);
				return;
			}
		}
		nanosleep(&worker_threads_check_period, NULL); // Add a little delay to give the CPU some rest.
	}
}

static bool
at_least_one_thread_is_running(void)
{
	for (size_t i = 0; i < worker_threads_count; ++i) {
		if (worker_threads[i].was_started == true && worker_threads[i].says_it_is_done == false) {
			return true;
		}
	}
	return false;
}

static void *
queue_worker(void *dummy)
{
	(void)dummy;
	const struct timespec delay_interval = {0, 200000000}; // 0.2 seconds
	const size_t sleeps_to_auto_update = 300;
	size_t sleep_iter = 0;

	while (they_want_us_to_terminate == false) {
		if (update_queue_length == 0) {
			if (sleep_iter == 0) {
				process_auto_updating_feeds();
			}
			sleep_iter = (sleep_iter + 1) % sleeps_to_auto_update;
			nanosleep(&delay_interval, NULL);
			continue;
		}
		prevent_status_cleaning();
		pthread_mutex_lock(&queue_lock);
here_we_go_again:
		while (they_want_us_to_terminate == false && update_queue_progress != update_queue_length) {
			struct feed_entry *feed = update_queue[update_queue_progress];
			pthread_mutex_unlock(&queue_lock);
			branch_update_feed_action_into_thread(feed);
			pthread_mutex_lock(&queue_lock);
			update_queue_progress += 1;
		}
		pthread_mutex_unlock(&queue_lock);

		while (update_queue_progress == update_queue_length && at_least_one_thread_is_running() == true) {
			nanosleep(&delay_interval, NULL);
		}

		pthread_mutex_lock(&queue_lock);
		if (they_want_us_to_terminate == false && update_queue_progress != update_queue_length) {
			// Something was added to the queue while the last feeds were updating.
			goto here_we_go_again;
		}

		tell_items_menu_to_regenerate();
		allow_status_cleaning();
		if (update_queue_failures == 0) {
			status_clean();
		} else {
			fail_status("Failed to update %zu feeds (check status history for details)", update_queue_failures);
		}
		if (cumulative_new_items_count > 0) {
			struct format_arg notification_cmd_args[] = {
				{L'q',  L'd',  {.i = cumulative_new_items_count}},
				{L'\0', L'\0', {.i = 0 /* terminator */        }},
			};
			run_command_with_specifiers(get_cfg_wstring(CFG_NOTIFICATION_COMMAND), notification_cmd_args);
		}

		free(update_queue);
		update_queue = NULL;
		update_queue_length = 0;
		update_queue_progress = 0;
		update_queue_failures = 0;
		updates_finished = 0;
		cumulative_new_items_count = 0;

		pthread_mutex_unlock(&queue_lock);
		nanosleep(&delay_interval, NULL);
	}

	return NULL;
}

void
update_feeds(struct feed_entry **feeds, size_t feeds_count)
{
	pthread_mutex_lock(&queue_lock);
	size_t old_update_queue_length = update_queue_length;
	struct feed_entry **temp = realloc(update_queue, sizeof(struct feed_entry *) * (update_queue_length + feeds_count));
	if (temp != NULL) {
		update_queue = temp;
		for (size_t i = 0; i < feeds_count; ++i) {
			bool already_present_in_queue = false;
			for (size_t j = 0; j < update_queue_length && already_present_in_queue == false; ++j) {
				if (feeds[i] == update_queue[j]) {
					already_present_in_queue = true;
				}
			}
			if (already_present_in_queue == false) {
				update_queue[update_queue_length++] = feeds[i];
			}
		}
	}
	if (update_queue_length != old_update_queue_length) {
		info_status("Feed updates completed: %zu/%zu", updates_finished, update_queue_length);
	}
	pthread_mutex_unlock(&queue_lock);
}

bool
start_feed_updater(void)
{
	worker_threads_count = get_cfg_uint(CFG_UPDATE_THREADS_COUNT);
	if (pthread_create(&queue_worker_thread, NULL, &queue_worker, NULL) != 0) {
		fputs("Failed to start feed updater!\n", stderr);
		return false;
	}
	return true;
}

void
stop_feed_updater(void)
{
	pthread_join(queue_worker_thread, NULL);
	for (size_t i = 0; i < worker_threads_count; ++i) {
		if (worker_threads[i].was_started == true) {
			pthread_join(worker_threads[i].thread, NULL);
		}
	}
	free(update_queue);
}
