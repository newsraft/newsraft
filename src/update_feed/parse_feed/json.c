#include "update_feed/parse_feed/parse_feed.h"

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

#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
	// Currently JSON Feed is the only JSON feed format supported.
	if (true) {
		json_dump_jsonfeed(json, &data);
	}
#endif

	cJSON_Delete(json);

	return true;
}
