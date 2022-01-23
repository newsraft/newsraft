#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	struct getfeed_data *data = getfeed_get_feed(url->ptr);
	if (data == NULL) {
		return false;
	}

	if (db_begin_transaction() == false) {
		getfeed_free_feed(data);
		return false;
	}

	if (insert_feed(url, data->feed) == false) {
		db_rollback_transaction();
		getfeed_free_feed(data);
		return false;
	}

	struct getfeed_item *item = data->item;
	while (item != NULL) {
		insert_item(url, item);
		item = item->next;
	}

	getfeed_free_feed(data);
	if (cfg.max_items != 0) {
		delete_excess_items(url);
	}

	if (db_commit_transaction() == false) {
		return false;
	}

	return true;
}
