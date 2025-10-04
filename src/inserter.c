#include <signal.h>
#include <string.h>
#include <strings.h>
#include "newsraft.h"

static bool
engage_with_not_inserted_feed(struct feed_update_state *data)
{
	if (data->is_finished == false
		&& (data->is_downloaded == true
		|| data->is_canceled == true
		|| data->is_failed == true))
	{
		return true;
	}
	return false;
}

void *
inserter_worker(void *dummy)
{
	(void)dummy;

	time_t last_auto_update = 0;

	while (they_want_us_to_stop == false) {
		if (time(NULL) - last_auto_update > 60) {
			process_auto_updating_feeds();
			last_auto_update = time(NULL);
		}

		struct feed_update_state *target = queue_pull(&engage_with_not_inserted_feed);
		if (target == NULL) {
			threads_take_a_nap(NEWSRAFT_THREAD_DBWRITER);
			continue;
		}

		struct feed_entry *feed = target->feed_entry;

		if (target->is_failed) {
			str_appendf(target->new_errors, "Feed update failed!\n");
		} else {
			if (target->is_canceled == false) {
				if (!insert_feed(feed, &target->feed)) {
					str_appendf(target->new_errors, "Feed insertion failed!\n");
				}
			} else {
				db_update_feed_int64(feed->url, "update_date", feed->update_date, true);

				// RFC9111, Section 4.3.4
				// For each stored response identified, the cache MUST update its
				// header fields with the header fields provided in the 304 (Not
				// Modified) response, as per Section 3.2.
				if (target->http_response_code == NEWSRAFT_HTTP_NOT_MODIFIED) {
					db_update_feed_string(feed->url, "http_header_etag", target->feed.http_header_etag, true);
					db_update_feed_int64(feed->url,  "http_header_last_modified", target->feed.http_header_last_modified, true);
					db_update_feed_int64(feed->url,  "http_header_expires", target->feed.http_header_expires, true);
				}
			}
		}

		// Interface routines actively access feed's name and errors strings in other threads.
		// The lock is here to prevent race condition which can potentially lead to segmentation fault.
		// TODO: make it less ugly perhaps?
		pthread_mutex_lock(&interface_lock);

		bool need_redraw = false;

		if (target->new_errors->len == 0) {
			if (feed->errors->len > 0) {
				need_redraw = true;
			}
			empty_string(feed->errors);
		} else {
			if (feed->errors->len == 0) {
				need_redraw = true;
			}
			catss(feed->errors, target->new_errors);
		}

		size_t errors_len_before_count = feed->errors->len;
		int64_t new_unread_count = db_count_items(&feed, 1, true);
		int64_t new_items_count  = db_count_items(&feed, 1, false);
		size_t errors_len_after_count = feed->errors->len;

		if (new_unread_count >= 0 && new_unread_count != feed->unread_count) {
			if (new_unread_count > feed->unread_count) {
				target->new_items_count = new_unread_count - feed->unread_count;
			}
			feed->unread_count = new_unread_count;
			need_redraw = true;
		}
		if (new_items_count >= 0 && new_items_count != feed->items_count) {
			feed->items_count = new_items_count;
			need_redraw = true;
		}
		if (errors_len_before_count != errors_len_after_count) {
			need_redraw = true;
		}

		if (STRING_IS_EMPTY(feed->name)) {
			struct string *title = db_get_string_from_feed_table(feed->url, "title", 5);
			if (title != NULL) {
				inlinefy_string(title);
				cpyss(&feed->name, title);
				free_string(title);
				need_redraw = true;
			}
		}

		pthread_mutex_unlock(&interface_lock);

		if (ui_is_running() && need_redraw == true) {
			refresh_sections_statistics_about_underlying_feeds();
			expose_all_visible_entries_of_the_list_menu();
		}

		target->is_finished = true;
		queue_examine();
	}
	return NULL;
}
