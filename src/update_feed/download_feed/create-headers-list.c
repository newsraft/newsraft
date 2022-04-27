#include "update_feed/download_feed/download_feed.h"

struct curl_slist *
create_list_of_headers(struct getfeed_feed *feed)
{
	struct curl_slist *headers = NULL;

	// A-IM
	headers = curl_slist_append(headers, "A-IM: feed");
	INFO("Attached header: A-IM: feed");

	// If-None-Match
	if ((cfg.send_if_none_match_header == true) && (feed->etag_header->len != 0)) {
		struct string *if_none_match_header = crtas("If-None-Match: \"", 16);
		catss(if_none_match_header, feed->etag_header);
		catcs(if_none_match_header, '"');
		headers = curl_slist_append(headers, if_none_match_header->ptr);
		INFO("Attached header: %s", if_none_match_header->ptr);
		free_string(if_none_match_header);
	}

	// If-Modified-Since
	if ((cfg.send_if_modified_since_header == true) && (feed->last_modified_header->len != 0)) {
		struct string *if_modified_since_header = crtas("If-Modified-Since: ", 19);
		catss(if_modified_since_header, feed->last_modified_header);
		headers = curl_slist_append(headers, if_modified_since_header->ptr);
		INFO("Attached header: %s", if_modified_since_header->ptr);
		free_string(if_modified_since_header);
	}

	return headers;
}
