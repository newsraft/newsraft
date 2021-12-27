#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static void
start_element_handler(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	if (data->error != PARSE_OKAY) {
		return;
	}
	++(data->depth);
	empty_string(data->value);
	if (parse_namespace_element_start(data, name, atts) == 0) {
		// Successfully processed an element by its namespace.
		return;
	}
	if (data->start_handler != NULL) {
		data->start_handler(data, name, atts);
		return;
	}
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	if ((data->depth == 1) && (strcmp(name, "rss") == 0)) {
		const char *version = get_value_of_attribute_key(atts, "version");
		if ((version != NULL) &&
			((strcmp(version, "2.0") == 0) ||
			(strcmp(version, "0.94") == 0) ||
			(strcmp(version, "0.93") == 0) ||
			(strcmp(version, "0.92") == 0) ||
			(strcmp(version, "0.91") == 0)))
		{
			data->start_handler = &parse_rss20_element_start;
			data->end_handler = &parse_rss20_element_end;
		}
	}
#endif
}

static void
end_element_handler(struct parser_data *data, const XML_Char *name)
{
	if (data->error != PARSE_OKAY) {
		return;
	}
	--(data->depth);
	strip_whitespace_from_string(data->value);
	if (parse_namespace_element_end(data, name) == 0) {
		// Successfully processed an element by its namespace.
		return;
	}
	if (data->end_handler != NULL) {
		data->end_handler(data, name);
		return;
	}
}

// Important note: a single block of contiguous text free of markup may still result in a sequence of calls to CharacterDataHandler.
static void
character_data_handler(struct parser_data *data, const XML_Char *s, int s_len)
{
	if (data->error == PARSE_OKAY) {
		if (catas(data->value, s, s_len) != 0) {
			FAIL("Not enough memory for character data of element!");
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
}

static size_t
parse_stream_callback(char *contents, size_t length, size_t nmemb, struct parser_data *data)
{
	size_t real_size = length * nmemb;
	if (data->error == PARSE_OKAY) {
		if (XML_Parse(data->parser, contents, real_size, XML_FALSE) != XML_STATUS_OK) {
			FAIL("A parse error occurred in the middle of data!");
			FAIL("Parser reports \"%" XML_FMT_STR "\" at line %" XML_FMT_INT_MOD "u.",
			     XML_ErrorString(XML_GetErrorCode(data->parser)),
			     XML_GetCurrentLineNumber(data->parser));
			data->error = PARSE_FAIL_XML_PARSE_ERROR;
		}
	}
	return real_size;
}

// On success returns PARSE_OKAY (zero).
// On failure returns non-zero.
int
update_feed(const struct string *url)
{
	struct parser_data data;
	data.value        = create_string(NULL, config_init_parser_buf_size);
	if (data.value == NULL) {
		FAIL("Not enough memory for parser buffer to start updating a feed!");
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data.bucket        = create_item_bucket();
	if (data.bucket == NULL) {
		FAIL("Not enough memory for item bucket to start updating a feed!");
		free_string(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data.parser        = XML_ParserCreateNS(NULL, NAMESPACE_SEPARATOR);
	if (data.parser == NULL) {
		FAIL("Something went wrong during parser struct creation, can not start updating a feed!");
		free_item_bucket(data.bucket);
		free_string(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data.depth         = 0;
	data.feed_url      = url;
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	data.rss20_pos = RSS20_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	data.atom10_pos = ATOM10_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	data.atom03_pos = ATOM03_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	data.dc_pos = DC_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	data.rss11_pos = RSS11_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	data.rss10content_pos = RSS10CONTENT_NONE;
#endif
	data.start_handler = NULL;
	data.end_handler   = NULL;
	data.error         = PARSE_OKAY;

	XML_SetElementHandler(
		data.parser,
		(void (*)(void *, const XML_Char *, const XML_Char **))&start_element_handler,
		(void (*)(void *, const XML_Char *))&end_element_handler
	);
	XML_SetCharacterDataHandler(
		data.parser,
		(void (*)(void *, const XML_Char *, int))&character_data_handler
	);
	XML_SetUserData(data.parser, &data);

	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		FAIL("Something went wrong during curl easy handle creation, can not start updating a feed!");
		free_item_bucket(data.bucket);
		XML_ParserFree(data.parser);
		free_string(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url->ptr);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// Empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	char curl_errbuf[CURL_ERROR_SIZE] = "";
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);

	if (db_begin_transaction() != 0) {
		free_item_bucket(data.bucket);
		XML_ParserFree(data.parser);
		free_string(data.value);
		curl_easy_cleanup(curl);
		return PARSE_FAIL_DB_TRANSACTION_ERROR;
	}

	int res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		size_t curl_errbuf_len = strlen(curl_errbuf);
		if (curl_errbuf_len != 0) {
			if (curl_errbuf[curl_errbuf_len - 1] == '\n') {
				curl_errbuf[curl_errbuf_len - 1] = '\0';
			}
			WARN("curl_easy_perform failed: %s", curl_errbuf);
		} else {
			WARN("curl_easy_perform failed: %s", curl_easy_strerror(res));
		}
		WARN("Feed update stopped due to fail in curl request!");
		db_rollback_transaction();
		free_item_bucket(data.bucket);
		XML_ParserFree(data.parser);
		free_string(data.value);
		curl_easy_cleanup(curl);
		return PARSE_FAIL_CURL_EASY_PERFORM_ERROR;
	}

	if (data.error == PARSE_FAIL_NOT_ENOUGH_MEMORY) {
		FAIL("Feed update stopped due to shortage of memory!");
	} else if (data.error == PARSE_FAIL_XML_PARSE_ERROR) {
		FAIL("Feed update stopped due to parse error!");
	}

	if (XML_Parse(data.parser, NULL, 0, XML_TRUE) != XML_STATUS_OK) {
		FAIL("Last call of XML_Parse failed!");
		if (data.error == PARSE_OKAY) {
			FAIL("Parser reports \"%" XML_FMT_STR "\" at line %" XML_FMT_INT_MOD "u.",
			     XML_ErrorString(XML_GetErrorCode(data.parser)),
			     XML_GetCurrentLineNumber(data.parser));
			data.error = PARSE_FAIL_XML_PARSE_ERROR;
		}
	}

	if (data.error == PARSE_OKAY) {
		db_commit_transaction();
	} else {
		db_rollback_transaction();
	}

	free_item_bucket(data.bucket);
	XML_ParserFree(data.parser);
	free_string(data.value);
	curl_easy_cleanup(curl);
	return data.error;
}
