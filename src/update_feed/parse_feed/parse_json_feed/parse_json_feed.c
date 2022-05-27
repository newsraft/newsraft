#include "update_feed/parse_feed/parse_json_feed/parse_json_feed.h"

bool
parse_json_feed(const struct string *feed_buf, struct getfeed_feed *feed)
{
	struct json_data data;
	data.feed = feed;

	cJSON *json = cJSON_ParseWithLength(feed_buf->ptr, feed_buf->len);
	if (json == NULL) {
		// Call destructor anyways because cJSON has some
		// magic going on "behind the doors" (static variables stuff).
		cJSON_Delete(json);
		return false;
	}

	// Currently JSON Feed is the only JSON feed format supported.
#ifdef NEWSRAFT_FORMAT_SUPPORT_JSONFEED
	json_dump_jsonfeed(json, &data);
#else
	cJSON_Delete(json);
	return false;
#endif

	cJSON_Delete(json);

	return true;
}
