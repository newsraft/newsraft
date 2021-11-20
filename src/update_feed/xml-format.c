#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static void
character_data_handler(void *userData, const XML_Char *s, int s_len)
{
	struct parser_data *data = userData;
	if (data->prev_pos != data->pos) data->value_len = 0;
	if (data->value_len + s_len > data->value_lim) {
		// multiply by 2 to minimize number of further realloc calls
		data->value_lim = (data->value_len + s_len) * 2;
		data->value = realloc(data->value, data->value_lim + 1);
		if (data->value == NULL) {
			data->fail = true;
			return;
		}
	}
	memcpy(data->value + data->value_len, s, s_len);
	data->value_len += s_len;
	*(data->value + data->value_len) = '\0';
	data->prev_pos = data->pos;
}

static size_t
parse_stream_callback(void *contents, size_t length, size_t nmemb, void *userp)
{
	XML_Parser parser = (XML_Parser)userp;
	struct parser_data *data = (struct parser_data *)XML_GetUserData(parser);
	size_t real_size = length * nmemb;
	if (data->fail == false && XML_Parse(parser, contents, real_size, (XML_Bool)false) == 0) {
		debug_write(DBG_FAIL, "Parsing response failed: %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
		data->fail = true;
	}
	return real_size;
}

static void XMLCALL
process_element_start(void *userData, const XML_Char *name, const XML_Char **atts) {
	struct parser_data *data = userData;
	++(data->depth);
	if (data->depth != 1) {
		return;
	}
	if (strcmp(name, "rss") == 0) {
		for (size_t i = 0; atts[i] != NULL; ++i) {
			if (strcmp(atts[i], "version") == 0) {
				++i;
				if (atts[i] != NULL &&
					(strcmp(atts[i], "2.0") == 0 ||
					strcmp(atts[i], "0.94") == 0 ||
					strcmp(atts[i], "0.93") == 0 ||
					strcmp(atts[i], "0.92") == 0 ||
					strcmp(atts[i], "0.91") == 0))
				{
					XML_SetElementHandler(*(data->parser), &parse_rss20_element_beginning, &parse_rss20_element_end);
					return;
				}
				break;
			}
		}
	}
	XML_SetElementHandler(*(data->parser), &parse_generic_element_beginning, &parse_generic_element_end);
}

static void XMLCALL
process_element_finish(void *userData, const XML_Char *name) {
	(void)name;
	struct parser_data *data = userData;
	--(data->depth);
}

int
update_feed(const struct string *url)
{
	int error = 0;

	struct item_bucket *bucket = create_item_bucket();
	if (bucket == NULL) {
		debug_write(DBG_FAIL, "Could not allocate enough memory for item bucket to start updating a feed!\n");
		error = 1;
		return error; // failure
	}

	XML_Parser parser = XML_ParserCreateNS(NULL, NAMESPACE_SEPARATOR);
	struct parser_data data = {
		.value     = malloc(sizeof(char) * config_init_parser_buf_size),
		.value_len = 0,
		.value_lim = config_init_parser_buf_size,
		.depth     = 0,
		.pos       = IN_ROOT,
		.prev_pos  = IN_ROOT,
		.feed_url  = url,
		.bucket    = bucket,
		.parser    = &parser,
		.fail      = false,
	};
	XML_SetUserData(parser, &data);
	XML_SetCharacterDataHandler(parser, &character_data_handler);
	XML_SetElementHandler(parser, &process_element_start, &process_element_finish);

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url->ptr);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)parser);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	/*char curl_errbuf[CURL_ERROR_SIZE];*/
	/*curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);*/

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
	free(data.value);
	XML_ParserFree(parser);
	curl_easy_cleanup(curl);
	return error;
}
