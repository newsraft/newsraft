#ifndef PARSE_FEED_H
#define PARSE_FEED_H
#include "update_feed/update_feed.h"

enum update_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

time_t parse_date_rfc822(const struct string *value);
time_t parse_date_rfc3339(const char *src, size_t src_len);

// See "parse_xml_feed" directory for implementation.
bool parse_xml_feed(const struct string *feed_buf, struct getfeed_feed *feed);

// See "parse_json_feed" directory for implementation.
bool parse_json_feed(const struct string *feed_buf, struct getfeed_feed *feed);
#endif // PARSE_FEED_H
