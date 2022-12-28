#include "update_feed/update_feed.h"

static struct feed_entry **update_queue = NULL;
static size_t update_queue_length = 0;
static size_t update_queue_progress = 0;
static size_t update_queue_fails_count;

static pthread_t queue_worker_thread;
static bool queue_worker_is_active = false;
static bool queue_worker_was_active_at_least_once = false;

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

	if (time((time_t *)&data.feed.download_date) == (time_t)-1) {
		goto undo;
	}

	if (get_cfg_bool(CFG_RESPECT_EXPIRES_HEADER) == true) {
		int64_t expires_date = db_get_date_from_feeds_table(feed->link, "http_header_expires", 19);
		if (expires_date < 0) {
			goto undo;
		} else if ((expires_date > 0) && (data.feed.download_date < expires_date)) {
			INFO("Content hasn't expired yet - aborting update without error.");
			status = DOWNLOAD_CANCELED;
			goto undo;
		}
	}

	if (get_cfg_bool(CFG_RESPECT_TTL_ELEMENT) == true) {
		int64_t ttl = db_get_date_from_feeds_table(feed->link, "time_to_live", 12);
		int64_t prev_download_date = db_get_date_from_feeds_table(feed->link, "download_date", 13);
		if ((ttl < 0) || (prev_download_date < 0)) {
			goto undo;
		}
		if ((ttl > 0) && (prev_download_date > 0) && ((prev_download_date + ttl) > data.feed.download_date)) {
			INFO("Content isn't dead yet - aborting update without error.");
			status = DOWNLOAD_CANCELED;
			goto undo;
		}
	}

	if (get_cfg_bool(CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) {
		data.feed.http_header_last_modified = db_get_date_from_feeds_table(feed->link, "http_header_last_modified", 25);
		if (data.feed.http_header_last_modified < 0) {
			// Error message written by db_get_date_from_feeds_table.
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
		update_queue_fails_count++;
	} else if (status == DOWNLOAD_CANCELED) {
		INFO("Download canceled.");
	}
	pthread_mutex_unlock(&update_lock);
	indicate_that_thread_routine_has_finished(feed->update_thread_index);
	return NULL;
}

static void *
start_processing_queue(void *arg)
{
	(void)arg;
	update_queue_progress = 0;
	update_queue_fails_count = 0;

	prevent_status_cleaning();

here_we_go_again:
	while (update_queue_progress != update_queue_length) {
		branch_update_feed_action_into_thread(&update_feed_action, update_queue[update_queue_progress]);
		pthread_mutex_lock(&queue_lock);
		info_status("(%zu/%zu) Loading %s", update_queue_progress + 1, update_queue_length, update_queue[update_queue_progress]->link->ptr);
		update_queue_progress += 1;
		pthread_mutex_unlock(&queue_lock);
	}

	wait_for_all_threads_to_finish();

	pthread_mutex_lock(&queue_lock);
	if (update_queue_progress != update_queue_length) {
		// Something was added to the queue while the last feeds were updating.
		pthread_mutex_unlock(&queue_lock);
		goto here_we_go_again;
	}
	queue_worker_is_active = false;
	update_queue_length = 0;
	update_queue_progress = 0;
	free(update_queue);
	update_queue = NULL;
	pthread_mutex_unlock(&queue_lock);

	allow_status_cleaning();

	if (update_queue_fails_count != 0) {
		fail_status("Failed to update %zu feeds (check out status history for more details)", update_queue_fails_count);
	} else {
		status_clean();
	}
	return NULL;
}

void
update_feeds(struct feed_entry **feeds, size_t feeds_count)
{
	pthread_mutex_lock(&queue_lock);
	struct feed_entry **temp = realloc(update_queue, sizeof(struct feed_entry *) * (update_queue_length + feeds_count));
	if (temp != NULL) {
		update_queue = temp;
		for (size_t i = 0; i < feeds_count; ++i) {
			update_queue[update_queue_length++] = feeds[i];
		}
	}
	info_status("(%zu/%zu) Loading %s", update_queue_progress + 1, update_queue_length, update_queue[update_queue_progress]->link->ptr);
	pthread_mutex_unlock(&queue_lock);

	if (queue_worker_is_active == false) {
		if (queue_worker_was_active_at_least_once == true) {
			pthread_join(queue_worker_thread, NULL);
		}
		queue_worker_is_active = true;
		pthread_create(&queue_worker_thread, NULL, &start_processing_queue, NULL);
		queue_worker_was_active_at_least_once = true;
	}
}
