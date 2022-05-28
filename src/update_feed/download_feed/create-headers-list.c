#include "update_feed/download_feed/download_feed.h"

struct curl_slist *
create_list_of_headers(const struct getfeed_feed *feed)
{
	struct curl_slist *headers = NULL;

	// A-IM
	headers = curl_slist_append(headers, "A-IM: feed");
	INFO("Attached header: A-IM: feed");

	// If-None-Match
	if ((get_cfg_bool(CFG_SEND_IF_NONE_MATCH_HEADER) == true) && (feed->http_header_etag->len != 0)) {
		struct string *if_none_match_header = crtas("If-None-Match: \"", 16);
		// TODO null check
		catss(if_none_match_header, feed->http_header_etag);
		catcs(if_none_match_header, '"');
		headers = curl_slist_append(headers, if_none_match_header->ptr);
		INFO("Attached header: %s", if_none_match_header->ptr);
		free_string(if_none_match_header);
	}

	// If-Modified-Since
	if ((get_cfg_bool(CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) && (feed->http_header_last_modified > 0)) {
		struct string *if_modified_since_header = crtas("If-Modified-Since: ", 19);
		// TODO null check
		struct string *http_date = get_http_date_str(feed->http_header_last_modified);
		// TODO null check
		catss(if_modified_since_header, http_date);
		headers = curl_slist_append(headers, if_modified_since_header->ptr);
		INFO("Attached header: %s", if_modified_since_header->ptr);
		free_string(if_modified_since_header);
		free_string(http_date);
	}

	return headers;
}
