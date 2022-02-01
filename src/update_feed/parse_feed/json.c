#include "update_feed/parse_feed/parse_feed.h"

struct getfeed_feed *
parse_json_feed(const struct string *feed_buf)
{
	struct json_data data;
	data.feed = create_feed();
	if (data.feed == NULL) {
		return NULL;
	}
#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
	data.jsonfeed_pos = JSONFEED_NONE;
#endif

	cJSON *json = cJSON_ParseWithLength(feed_buf->ptr, feed_buf->len);
	if (json == NULL) {
		// Call destructor anyways because cJSON has some
		// magic going on "behind the doors" (static variables stuff).
		cJSON_Delete(json);
		free_feed(data.feed);
		return NULL;
	}

#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
	// Currently JSON Feed is the only JSON feed format supported.
	if (true) {
		json_dump_jsonfeed(json, &data);
	}
#endif

	cJSON_Delete(json);

	return data.feed;
}
