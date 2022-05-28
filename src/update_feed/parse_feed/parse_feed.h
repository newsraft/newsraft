#ifndef PARSE_FEED_H
#define PARSE_FEED_H
#include "update_feed/update_feed.h"

time_t parse_date_rfc822(const struct string *value);
time_t parse_date_rfc3339(const char *src, size_t src_len);
#endif // PARSE_FEED_H
