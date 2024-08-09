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
			wait_a_second_for_wake_up_signal();
			continue;
		}

		struct feed_entry *feed = target->feed_entry;

		if (target->is_failed == false && target->is_canceled == false) {
			insert_feed(target->feed_entry, &target->feed);
		}

		bool need_redraw = false;
		int64_t new_unread_count = get_unread_items_count_of_the_feed(feed->link);
		if (new_unread_count >= 0 && new_unread_count != feed->unread_count) {
			if (new_unread_count > feed->unread_count) {
				target->new_items_count = new_unread_count - feed->unread_count;
			}
			feed->unread_count = new_unread_count;
			refresh_unread_items_count_of_all_sections();
			need_redraw = true;
		}

		if (feed->name == NULL || feed->name->len == 0) {
			struct string *title = db_get_string_from_feed_table(feed->link, "title", 5);
			if (title != NULL) {
				inlinefy_string(title);
				cpyss(&feed->name, title);
				free_string(title);
				need_redraw = true;
			}
		}

		if (target->is_canceled) {
			db_set_update_date(feed->link, feed->update_date);
		}
		if (need_redraw == true) {
			expose_all_visible_entries_of_the_list_menu();
		}
		target->is_finished = true;
		queue_examine();
	}
	return NULL;
}
