#ifndef PARSE_JSON_FEED_H
#define PARSE_JSON_FEED_H
#include <cjson/cJSON.h>
#include "update_feed/parse_feed/parse_feed.h"

struct json_data {
	struct getfeed_feed *feed;
};

#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
void json_dump_jsonfeed(cJSON *json, struct json_data *data);
#endif
#endif // PARSE_JSON_FEED_H
