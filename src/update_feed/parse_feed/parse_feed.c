#include "update_feed/parse_feed/parse_feed.h"

bool
parse_feed(const struct string *feed_buf, struct getfeed_feed *feed)
{
	const char *first_char = feed_buf->ptr;
	while (ISWHITESPACE(*first_char)) {
		++first_char;
	}
	if (*first_char == '{') {
		INFO("First character of the feed buffer is '{', parsing in JSON mode.");
		return parse_json_feed(feed_buf, feed);
	} else {
		INFO("First character of the feed buffer is not '{', parsing in XML mode.");
		return parse_xml_feed(feed_buf, feed);
	}
}
