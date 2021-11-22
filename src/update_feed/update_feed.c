#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static struct parser_data data;

static void XMLCALL
start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts) {
	(void)userData;
	++(data.depth);
	data.value_len = 0;
	if (data.start_handle != NULL) {
		data.start_handle(&data, name, atts);
		return;
	}
	if (data.depth != 1) {
		return;
	}
	const char *version;
	if (strcmp(name, "rss") == 0) {
		version = get_value_of_attribute_key(atts, "version");
		if ((version != NULL) &&
		    ((strcmp(version, "2.0") == 0)  ||
		     (strcmp(version, "0.94") == 0) ||
		     (strcmp(version, "0.93") == 0) ||
		     (strcmp(version, "0.92") == 0) ||
		     (strcmp(version, "0.91") == 0)))
		{
			data.start_handle = &parse_rss20_element_start;
			data.end_handle = &parse_rss20_element_end;
			return;
		}
	}
	data.start_handle = &parse_generic_element_start;
	data.end_handle = &parse_generic_element_end;
}

static void XMLCALL
end_element_handler(void *userData, const XML_Char *name) {
	(void)userData;
	--(data.depth);
	if (data.end_handle != NULL) {
		data.end_handle(&data, name);
		return;
	}
}

static void XMLCALL
character_data_handler(void *userData, const XML_Char *s, int s_len)
{
	// Important note: a single block of contiguous text free of markup may still result in a sequence of calls to CharacterDataHandler.
	(void)userData;
	if (data.value_len + s_len > data.value_lim) {
		// multiply by 2 to minimize number of further realloc calls
		data.value_lim = (data.value_len + s_len) * 2;
		data.value = realloc(data.value, data.value_lim + 1);
		if (data.value == NULL) {
			data.fail = true;
			return;
		}
	}
	memcpy(data.value + data.value_len, s, s_len);
	data.value_len += s_len;
	*(data.value + data.value_len) = '\0';
}

static size_t
parse_stream_callback(void *contents, size_t length, size_t nmemb, void *userp)
{
	(void)userp;
	size_t real_size = length * nmemb;
	if (data.fail == false && XML_Parse(data.parser, contents, real_size, (XML_Bool)false) == 0) {
		debug_write(DBG_FAIL, "Parsing response failed: %s\n", XML_ErrorString(XML_GetErrorCode(data.parser)));
		data.fail = true;
	}
	return real_size;
}

int
update_feed(const struct string *url)
{
	struct item_bucket *bucket = create_item_bucket();
	if (bucket == NULL) {
		debug_write(DBG_FAIL, "Could not allocate enough memory for item bucket to start updating a feed!\n");
		return 1; // failure
	}

	XML_Parser parser = XML_ParserCreateNS(NULL, NAMESPACE_SEPARATOR);
	if (parser == NULL) {
		debug_write(DBG_FAIL, "Something went wrong during parser struct creation, can not start updating a feed!\n");
		free_item_bucket(bucket);
		return 1; // failure
	}

	data.value        = malloc(sizeof(char) * config_init_parser_buf_size);
	if (data.value == NULL) {
		debug_write(DBG_FAIL, "Could not allocate enough memory for parser buffer to start updating a feed!\n");
		free_item_bucket(bucket);
		XML_ParserFree(parser);
		return 1; // failure
	}
	data.value_len    = 0;
	data.value_lim    = config_init_parser_buf_size;
	data.depth        = 0;
	data.pos          = IN_ROOT;
	data.feed_url     = url;
	data.bucket       = bucket;
	data.parser       = parser;
	data.start_handle = NULL;
	data.end_handle   = NULL;
	data.fail         = false;

	XML_SetElementHandler(parser, &start_element_handler, &end_element_handler);
	XML_SetCharacterDataHandler(parser, &character_data_handler);

	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		debug_write(DBG_FAIL, "Something went wrong during curl easy handle creation, can not start updating a feed!\n");
		free_item_bucket(bucket);
		XML_ParserFree(parser);
		free(data.value);
		return 1; // failure
	}
	curl_easy_setopt(curl, CURLOPT_URL, url->ptr);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// Empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	/*char curl_errbuf[CURL_ERROR_SIZE];*/
	/*curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);*/

	int error = 0;

	int res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		status_write("[fail] %s", url->ptr);
		debug_write(DBG_WARN, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		error = 1;
	} else if (data.fail == true) {
		status_write("[fail] %s", url->ptr);
		error = 1;
	} else {
		/* final XML_Parse */
		if (XML_Parse(parser, NULL, 0, (XML_Bool)true) == XML_STATUS_ERROR) {
			status_write("[invalid format] %s", url->ptr);
			debug_write(DBG_FAIL, "%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",
			            XML_ErrorString(XML_GetErrorCode(parser)),
			            XML_GetCurrentLineNumber(parser));
			error = 1;
		}
	}

	free_item_bucket(bucket);
	XML_ParserFree(parser);
	free(data.value);
	curl_easy_cleanup(curl);
	return error;
}
