#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static void XMLCALL
process_element_start(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct init_parser_data *parser_data = userData;
	++(parser_data->depth);
	if (parser_data->depth == 1 && parser_data->parser_func == NULL) {
		if (strcmp(name, "rss") == 0) {
			int8_t i;
			for (i = 0; atts[i] != NULL && strcmp(atts[i], "version") != 0; ++i) {}
			if (atts[i] != NULL) ++i;
			if (atts[i] != NULL &&
			    (strcmp(atts[i], "2.0") == 0 ||
			    strcmp(atts[i], "0.94") == 0 ||
			    strcmp(atts[i], "0.93") == 0 ||
			    strcmp(atts[i], "0.92") == 0 ||
			    strcmp(atts[i], "0.91") == 0))
			{
				parser_data->parser_func = &parse_rss20;
			}
		} else {
			parser_data->parser_func = &parse_generic;
		}
		if (parser_data->parser_func != NULL) XML_StopParser(*(parser_data->xml_parser), (XML_Bool)true);
	}
}

static void XMLCALL
process_element_finish(void *userData, const XML_Char *name) {
	struct init_parser_data *parser_data = userData;
	--(parser_data->depth);
}

int
feed_process(struct string *buf, struct feed_entry *feed)
{
	XML_Parser parser = XML_ParserCreateNS(NULL, ':');
	struct init_parser_data parser_data = {
		.depth = 0,
		.xml_parser = &parser,
		.parser_func = NULL,
	};
	XML_SetUserData(parser, &parser_data);
	XML_SetElementHandler(parser, &process_element_start, &process_element_finish);
	if (XML_Parse(parser, buf->ptr, buf->len, 0) == XML_STATUS_ERROR) {
		/*status_write("%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
								 /*XML_ErrorString(XML_GetErrorCode(parser)),*/
								 /*XML_GetCurrentLineNumber(parser));*/
		status_write("[invalid format] %s", feed->feed_url->ptr); // bad xml document
		XML_ParserFree(parser);
		return 1;
	}
	if (parser_data.parser_func == NULL) {
		status_write("[unknown format] %s", feed->feed_url->ptr);
		return 1;
	}

	struct item_bucket new_bucket = {0};
	struct feed_parser_data feed_data = {
		.value     = malloc(sizeof(char) * INIT_PARSER_BUF_SIZE),
		.value_len = 0,
		.value_lim = INIT_PARSER_BUF_SIZE,
		.depth     = 0,
		.pos       = 0,
		.prev_pos  = 0,
		.feed_url  = feed->feed_url,
		.bucket    = &new_bucket
	};
	XML_SetUserData(parser, &feed_data);
	int parsing_error = parser_data.parser_func(&parser, feed->feed_url, &feed_data);
	free(feed_data.value);
	XML_ParserFree(parser);
	if (parsing_error != 0) {
		/*status_write("[incorrect format] %s", feed->feed_url->ptr);*/
		return 1;
	}

	return 0;
}

time_t
get_unix_epoch_time(char *format_str, char *date_str)
{
	struct tm t = {0};
	if (strptime(date_str, format_str, &t) != NULL) {
		return mktime(&t);
	}
	return 0;
}

void
reset_item_bucket(struct item_bucket *bucket)
{
	if (bucket == NULL) return;
	if (bucket->guid != NULL) free_string(&bucket->guid);
	if (bucket->title != NULL) free_string(&bucket->title);
	if (bucket->url != NULL) free_string(&bucket->url);
	if (bucket->author != NULL) free_string(&bucket->author);
	if (bucket->category != NULL) free_string(&bucket->category);
	if (bucket->comments != NULL) free_string(&bucket->comments);
	if (bucket->content != NULL) free_string(&bucket->content);
	bucket->pubdate = 0;
}

void
value_strip_whitespace(char *str, size_t *len)
{
	// strip whitespace from edges
	int i = 0, left_edge = 0, right_edge = *len - 1;
	while ((*(str + left_edge) == ' '   ||
	        *(str + left_edge) == '\t'  ||
	        *(str + left_edge) == '\r'  ||
	        *(str + left_edge) == '\n') &&
	       left_edge <= right_edge)
	{
		++left_edge;
	}
	while ((*(str + right_edge) == ' '   ||
	        *(str + right_edge) == '\t'  ||
	        *(str + right_edge) == '\r'  ||
	        *(str + right_edge) == '\n') &&
	       right_edge >= left_edge)
	{
		--right_edge;
	}
	if ((left_edge != 0) || (right_edge != *len - 1)) {
		if (right_edge < left_edge) {
			*(str + 0) = '\0';
			*len = 0;
			return;
		}
		for (; i <= right_edge - left_edge; ++i) {
			*(str + i) = *(str + i + left_edge);
		}
		*len = i;
		*(str + i) = '\0';
	}
}

void XMLCALL
store_xml_element_value(void *userData, const XML_Char *s, int s_len)
{
	struct feed_parser_data *data = userData;
	if (data->prev_pos != data->pos) data->value_len = 0;
	if (data->value_len + s_len > data->value_lim) {
		// multiply by 2 to minimize number of further realloc calls
		data->value_lim = (data->value_len + s_len) * 2;
		data->value = realloc(data->value, data->value_lim + 1);
		if (data->value == NULL) return;
	}
	memcpy(data->value + data->value_len, s, s_len);
	data->value_len += s_len;
	*(data->value + data->value_len) = '\0';
	data->prev_pos = data->pos;
}
