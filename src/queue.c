#include "newsraft.h"

struct queue_notification_category {
	const struct wstring *notify_cmd;
	size_t new_items_count;
};

static struct feed_update_state *update_queue = NULL;
static pthread_mutex_t update_queue_lock = PTHREAD_MUTEX_INITIALIZER;

struct feed_update_state *
queue_pull(bool (*condition)(struct feed_update_state *))
{
	pthread_mutex_lock(&update_queue_lock);
	for (struct feed_update_state *j = update_queue; j != NULL; j = j->next) {
		if (condition(j)) {
			pthread_mutex_unlock(&update_queue_lock);
			return j;
		}
	}
	pthread_mutex_unlock(&update_queue_lock);
	return NULL;
}

static void
destroy_queue_unprotected(void)
{
	struct feed_update_state *k = NULL;
	for (struct feed_update_state *j = update_queue; j != NULL; j = j->next, free(k)) {
		remove_downloader_handle(j);
		curl_easy_cleanup(j->curl);
		curl_slist_free_all(j->download_headers);
		if (j->media_type == MEDIA_TYPE_XML) {
			XML_ParserFree(j->xml_parser);
		} else if (j->media_type == MEDIA_TYPE_JSON) {
			yajl_free(j->json_parser);
		}
		free_string(j->feed.title);
		free_string(j->feed.url);
		free_string(j->feed.content);
		free_string(j->feed.attachments);
		free_string(j->feed.persons);
		free_string(j->feed.extras);
		free_string(j->feed.http_header_etag);
		free_string(j->text);
		free_item(j->feed.item);
		k = j;
	}
	update_queue = NULL;
}

void
queue_destroy(void)
{
	pthread_mutex_lock(&update_queue_lock);
	destroy_queue_unprotected();
	pthread_mutex_unlock(&update_queue_lock);
}

static void
queue_execute_update_notifications_unprotected(size_t max_units_count)
{
	struct queue_notification_category *units = calloc(max_units_count, sizeof(struct queue_notification_category));
	if (units == NULL) {
		return;
	}

	for (struct feed_update_state *i = update_queue; i != NULL; i = i->next) {
		const struct wstring *notify_cmd = get_cfg_wstring(&i->feed_entry->cfg, CFG_NOTIFICATION_COMMAND);
		if (notify_cmd && notify_cmd->len > 0 && i->new_items_count > 0) {
			struct queue_notification_category *unit = units;
			while (unit->notify_cmd != NULL && wcscmp(unit->notify_cmd->ptr, notify_cmd->ptr) != 0) {
				unit += 1;
			}
			unit->notify_cmd = notify_cmd;
			unit->new_items_count += i->new_items_count;
		}
	}

	for (size_t i = 0; i < max_units_count && units[i].notify_cmd != NULL; ++i) {
		struct format_arg notification_cmd_args[] = {
			{L'q',  L'd',  {.i = units[i].new_items_count}},
			{L'\0', L'\0', {.i = 0 /* terminator */}},
		};
		run_formatted_command(units[i].notify_cmd, notification_cmd_args);
	}

	free(units);
}

void
queue_examine(void)
{
	pthread_mutex_lock(&update_queue_lock);
	size_t update_queue_len = 0;
	size_t update_queue_failures = 0;
	size_t update_queue_cancelations = 0;
	size_t update_queue_finished_len = 0;
	for (struct feed_update_state *j = update_queue; j != NULL; j = j->next) {
		update_queue_len += 1;
		if (j->is_finished) {
			update_queue_finished_len += 1;
		}
		if (j->is_canceled) {
			update_queue_cancelations += 1;
		}
		if (j->is_failed && !get_cfg_bool(&j->feed_entry->cfg, CFG_SUPPRESS_ERRORS)) {
			update_queue_failures += 1;
		}
	}
	info_status("Feed updates completed: %zu/%zu", update_queue_finished_len, update_queue_len);
	if (update_queue_finished_len == update_queue_len) {
		tell_items_menu_to_regenerate();
		allow_status_cleaning();
		if (update_queue_failures > 0) {
			fail_status("%zu feeds failed (select failed feed and press v for more info)", update_queue_failures);
		} else if (update_queue_cancelations > 0) {
			info_status("%zu feeds are already up-to-date", update_queue_cancelations);
		} else {
			status_clean();
		}
		queue_execute_update_notifications_unprotected(update_queue_len);
		db_commit_transaction();
		destroy_queue_unprotected();
	} else {
		prevent_status_cleaning();
	}
	pthread_mutex_unlock(&update_queue_lock);
}

void
queue_updates(struct feed_entry **feeds, size_t feeds_count)
{
	time_t current_time = time(NULL);
	if (current_time <= 0) {
		FAIL("Failed to get system time!");
		return;
	}
	pthread_mutex_lock(&update_queue_lock);
	bool this_is_new_queue = update_queue == NULL ? true : false;
	for (size_t i = 0; i < feeds_count; ++i) {
		bool already_present_in_queue = false;
		for (struct feed_update_state *j = update_queue; j != NULL; j = j->next) {
			if (feeds[i] == j->feed_entry) {
				already_present_in_queue = true;
				break;
			}
		}
		if (already_present_in_queue == true) {
			continue;
		}

		feeds[i]->update_date = current_time;
		INFO("Feed %s update attempt date: %" PRId64, feeds[i]->link->ptr, feeds[i]->update_date);

		empty_string(feeds[i]->errors);

		struct feed_update_state *item = calloc(1, sizeof(struct feed_update_state));
		if (item == NULL) {
			return;
		}
		item->feed_entry = feeds[i];
		item->next = update_queue;
		update_queue = item;
	}
	if (this_is_new_queue) {
		db_begin_transaction();
	}
	pthread_mutex_unlock(&update_queue_lock);
	queue_examine();
	threads_wake_up(NEWSRAFT_THREAD_DOWNLOAD);
	threads_wake_up(NEWSRAFT_THREAD_SHRUNNER);
}

void
queue_wait_finish(void)
{
	bool complete = false;
	struct timespec check_period = {0, 100000000}; // 0.1 seconds
	do {
		nanosleep(&check_period, NULL);
		pthread_mutex_lock(&update_queue_lock);
		complete = update_queue == NULL ? true : false;
		pthread_mutex_unlock(&update_queue_lock);
	} while (complete == false);
}
