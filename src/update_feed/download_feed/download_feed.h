#ifndef DOWNLOAD_FEED_H
#define DOWNLOAD_FEED_H
#include <curl/curl.h>
#include "update_feed/update_feed.h"

// Unknown type must have 0 value!
enum {
	MEDIA_TYPE_UNKNOWN = 0,
	MEDIA_TYPE_XML,
	MEDIA_TYPE_JSON,
	MEDIA_TYPE_OTHER,
};

struct curl_slist *create_list_of_headers(const struct getfeed_feed *feed);
#endif // DOWNLOAD_FEED_H
