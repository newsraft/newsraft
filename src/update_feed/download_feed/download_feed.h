#ifndef DOWNLOAD_FEED_H
#define DOWNLOAD_FEED_H
#include <curl/curl.h>
#include "update_feed/update_feed.h"

struct curl_slist *create_list_of_headers(const struct getfeed_feed *feed);
#endif // DOWNLOAD_FEED_H
