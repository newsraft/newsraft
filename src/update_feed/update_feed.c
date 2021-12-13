#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static void XMLCALL
start_element_handler(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	if (data->error != PARSE_OKAY) {
		return;
	}
	++(data->depth);
	data->value_len = 0;
	if (data->start_handler != NULL) {
		if (parse_namespace_element_start(data, name, atts) != 0) {
			data->start_handler(data, name, atts);
		}
	} else {
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
				return;
			}
		} else {
			parse_namespace_element_start(data, name, atts);
		}
	}
}

static void XMLCALL
end_element_handler(struct parser_data *data, const XML_Char *name)
{
	if (data->error != PARSE_OKAY) {
		return;
	}
	--(data->depth);
	strip_whitespace_from_edges(data->value, &(data->value_len));
	if (data->end_handler == NULL) {
		parse_namespace_element_end(data, name);
		return;
	}
	data->end_handler(data, name);
}

// Important note: a single block of contiguous text free of markup may still result in a sequence of calls to CharacterDataHandler.
static void XMLCALL
character_data_handler(struct parser_data *data, const XML_Char *s, int s_len)
{
	if (data->error != PARSE_OKAY) {
		return;
	}
	if (data->value_len + s_len > data->value_lim) {
		// multiply by 2 to minimize number of further realloc calls
		data->value_lim = (data->value_len + s_len) * 2;
		char *tmp = realloc(data->value, sizeof(char) * (data->value_lim + 1));
		if (tmp == NULL) {
			FAIL("Not enough memory for element character data!");
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		data->value = tmp;
	}
	memcpy(data->value + data->value_len, s, s_len);
	data->value_len += s_len;
	*(data->value + data->value_len) = '\0';
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

static inline void
drop_position_indicators(void)
{
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	rss20_pos = 0;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	atom10_pos = 0;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	atom03_pos = 0;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	dc_pos = 0;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	rss11_pos = 0;
#endif
}

int
update_feed(const struct string *url)
{
	struct parser_data data;
	data.value        = malloc(sizeof(char) * config_init_parser_buf_size);
	if (data.value == NULL) {
		FAIL("Not enough memory for parser buffer to start updating a feed!");
		return PARSE_FAIL_NOT_ENOUGH_MEMORY; // failure
	}
	data.value_len     = 0;
	data.value_lim     = config_init_parser_buf_size - 1; // never forget about terminator
	data.depth         = 0;
	data.pos           = IN_ROOT;
	data.feed_url      = url;
	data.bucket        = create_item_bucket();
	if (data.bucket == NULL) {
		FAIL("Not enough memory for item bucket to start updating a feed!");
		free(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY; // failure
	}
	data.parser        = XML_ParserCreateNS(NULL, NAMESPACE_SEPARATOR);
	if (data.parser == NULL) {
		FAIL("Something went wrong during parser struct creation, can not start updating a feed!");
		free_item_bucket(data.bucket);
		free(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY; // failure
	}
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
		free(data.value);
		return PARSE_FAIL_NOT_ENOUGH_MEMORY; // failure
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

	drop_position_indicators();

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
		free_item_bucket(data.bucket);
		XML_ParserFree(data.parser);
		free(data.value);
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

	free_item_bucket(data.bucket);
	XML_ParserFree(data.parser);
	free(data.value);
	curl_easy_cleanup(curl);
	return data.error;
}
