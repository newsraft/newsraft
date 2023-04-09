#include "update_feed/update_feed.h"

static struct feed_entry **update_queue = NULL;
static size_t update_queue_length = 0;
static size_t update_queue_progress = 0;
static size_t update_queue_failures = 0;

static pthread_t queue_worker_thread;
static volatile bool queue_worker_is_asked_to_unlock = false;
static volatile bool queue_worker_is_asked_to_terminate = false;

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

	int8_t status = DOWNLOAD_FAILED;
	struct stream_callback_data data = {0};

	data.feed.download_date = time(NULL);
	if (data.feed.download_date <= 0) {
		FAIL("Failed to obtain system time!");
		goto undo;
	}

	if (get_cfg_bool(CFG_RESPECT_EXPIRES_HEADER) == true) {
		int64_t expires_date = db_get_date_from_feeds_table(feed->link, "http_header_expires", 19);
		if (expires_date < 0) {
			goto undo;
		} else if ((expires_date > 0) && (data.feed.download_date < expires_date)) {
			INFO("Aborting update because content hasn't expired yet.");
			status = DOWNLOAD_CANCELED;
			goto undo;
		}
	}

	if (get_cfg_bool(CFG_RESPECT_TTL_ELEMENT) == true) {
		int64_t ttl = db_get_date_from_feeds_table(feed->link, "time_to_live", 12);
		int64_t prev_download_date = db_get_date_from_feeds_table(feed->link, "download_date", 13);
		if ((ttl < 0) || (prev_download_date < 0)) {
			goto undo;
		} else if ((ttl > 0) && (prev_download_date > 0) && ((prev_download_date + ttl) > data.feed.download_date)) {
			INFO("Aborting update because content hasn't died yet.");
			status = DOWNLOAD_CANCELED;
			goto undo;
		}
	}

	if (get_cfg_bool(CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) {
		data.feed.http_header_last_modified = db_get_date_from_feeds_table(feed->link, "http_header_last_modified", 25);
		if (data.feed.http_header_last_modified < 0) {
			goto undo;
		}
	}

	if (get_cfg_bool(CFG_SEND_IF_NONE_MATCH_HEADER) == true) {
		data.feed.http_header_etag = db_get_string_from_feed_table(feed->link, "http_header_etag", 16);
	}

	status = download_feed(feed->link->ptr, &data);

	if (status == DOWNLOAD_SUCCEEDED) {
		if (insert_feed(feed->link, &data.feed) == false) {
			status = DOWNLOAD_FAILED;
		}
	}

	free_feed(&data.feed);

undo:
	pthread_mutex_lock(&update_lock);
	feed->download_date = data.feed.download_date;
	if (status == DOWNLOAD_SUCCEEDED) {
		bool need_redraw = false;
		int64_t new_unread_count = get_unread_items_count_of_the_feed(feed->link);
		if ((new_unread_count >= 0) && (new_unread_count != feed->unread_count)) {
			feed->unread_count = new_unread_count;
			refresh_unread_items_count_of_all_sections();
			need_redraw = true;
		}
		if ((feed->name == NULL) || (feed->name->len == 0)) {
			struct string *title = db_get_string_from_feed_table(feed->link, "title", 5);
			if (title != NULL) {
				inlinefy_string(title);
				crtss_or_cpyss(&feed->name, title);
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
	pthread_mutex_unlock(&update_lock);
	*feed->did_update_just_finished = true;
	return NULL;
}

static void *
queue_worker(void *arg)
{
	(void)arg;
	const struct timespec delay_interval = {0, 367879441}; // 367.88 ms

	while (queue_worker_is_asked_to_terminate == false) {
		pthread_mutex_lock(&queue_lock);
		if (update_queue_progress == update_queue_length) {
			pthread_mutex_unlock(&queue_lock);
			nanosleep(&delay_interval, NULL);
			continue;
		}
		prevent_status_cleaning();
here_we_go_again:
		while (queue_worker_is_asked_to_terminate == false) {
			if (update_queue_progress == update_queue_length) {
				break;
			}
			branch_update_feed_action_into_thread(&update_feed_action, update_queue[update_queue_progress]);
			info_status("(%zu/%zu) Loading %s", update_queue_progress + 1, update_queue_length, update_queue[update_queue_progress]->link->ptr);
			update_queue_progress += 1;
			if (queue_worker_is_asked_to_unlock == true) {
				// We unlock queue for a moment to let the user add new feeds to it.
				pthread_mutex_unlock(&queue_lock);
				nanosleep(&delay_interval, NULL);
				pthread_mutex_lock(&queue_lock);
			}
		}
		pthread_mutex_unlock(&queue_lock);

		wait_for_all_threads_to_finish();

		pthread_mutex_lock(&queue_lock);
		if ((queue_worker_is_asked_to_terminate == false)
			&& (update_queue_progress != update_queue_length))
		{
			// Something was added to the queue while the last feeds were updating.
			goto here_we_go_again;
		}

		tell_items_menu_to_regenerate();
		allow_status_cleaning();
		if (update_queue_failures != 0) {
			fail_status("Failed to update %zu feeds (check status history for details)", update_queue_failures);
		} else {
			status_clean();
		}
		free(update_queue);
		update_queue = NULL;
		update_queue_length = 0;
		update_queue_progress = 0;
		update_queue_failures = 0;
		pthread_mutex_unlock(&queue_lock);
		nanosleep(&delay_interval, NULL);
	}

	return NULL;
}

void
update_feeds(struct feed_entry **feeds, size_t feeds_count)
{
	queue_worker_is_asked_to_unlock = true;
	pthread_mutex_lock(&queue_lock);
	queue_worker_is_asked_to_unlock = false;
	struct feed_entry **temp = realloc(update_queue, sizeof(struct feed_entry *) * (update_queue_length + feeds_count));
	if (temp != NULL) {
		update_queue = temp;
		for (size_t i = 0; i < feeds_count; ++i) {
			bool already_present_in_queue = false;
			for (size_t j = 0; (j < update_queue_length) && (already_present_in_queue == false); ++j) {
				if (feeds[i] == update_queue[j]) {
					already_present_in_queue = true;
				}
			}
			if (already_present_in_queue == false) {
				update_queue[update_queue_length++] = feeds[i];
			}
		}
	}
	if (update_queue_progress < update_queue_length) {
		info_status("(%zu/%zu) Loading %s", update_queue_progress + 1, update_queue_length, update_queue[update_queue_progress]->link->ptr);
	}
	pthread_mutex_unlock(&queue_lock);
}

bool
start_feed_updater(void)
{
	if (initialize_update_threads() == false) {
		return false;
	}
	if (pthread_create(&queue_worker_thread, NULL, &queue_worker, NULL) != 0) {
		fputs("Failed to start feed updater!\n", stderr);
		terminate_update_threads();
		return false;
	}
	return true;
}

void
stop_feed_updater(void)
{
	queue_worker_is_asked_to_terminate = true;
	pthread_join(queue_worker_thread, NULL);
	terminate_update_threads();
	free(update_queue);
}
