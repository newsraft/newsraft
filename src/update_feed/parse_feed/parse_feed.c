#include "update_feed/parse_feed/parse_feed.h"

struct getfeed_feed *
parse_feed(const struct string *feed_buf)
{
	const char *first_char = feed_buf->ptr;
	while (ISWHITESPACE(*first_char)) {
		++first_char;
	}
	if (*first_char == '{') {
		return parse_json_feed(feed_buf);
	} else {
		return parse_xml_feed(feed_buf);
	}
}
