#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "feedeater.h"
#include "config.h"

enum {
	IN_ROOT = 0,
	IN_ITEM_ELEMENT = 1,
	IN_TITLE_ELEMENT = 2,
	IN_DESCRIPTION_ELEMENT = 4,
	IN_LINK_ELEMENT = 8,
	IN_PUBDATE_ELEMENT = 16,
	IN_GUID_ELEMENT = 32,
	IN_CATEGORY_ELEMENT = 64,
	IN_COMMENTS_ELEMENT = 128,
	IN_AUTHOR_ELEMENT = 256,
	IN_ENCLOSURE_ELEMENT = 512,
	IN_SOURCE_ELEMENT = 1024,
	IN_IMAGE_ELEMENT = 2048,
	IN_CHANNEL_ELEMENT = 4096,
};

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct feed_parser_data *data = userData;
	(data->depth)++;
	if      (strcmp(name, "item") == 0)        data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)       data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)        data->pos |= IN_LINK_ELEMENT;
	else if (strcmp(name, "pubDate") == 0)     data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "guid") == 0)        data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "category") == 0)    data->pos |= IN_CATEGORY_ELEMENT;
	else if (strcmp(name, "comments") == 0)    data->pos |= IN_COMMENTS_ELEMENT;
	else if (strcmp(name, "author") == 0)      data->pos |= IN_AUTHOR_ELEMENT;
	else if (strcmp(name, "channel") == 0)     data->pos |= IN_CHANNEL_ELEMENT;
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	/*(void)name;*/
	struct feed_parser_data *data = userData;
	(data->depth)--;
	if ((data->pos & IN_CHANNEL_ELEMENT) == 0) return;
	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		reset_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->title, data->value, sizeof(char) * data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->content, data->value, sizeof(char) * data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->url, data->value, sizeof(char) * data->value_len);
		else
			db_update_feed_text(data->feed_url, "resource", data->value, data->value_len);
	} else if (strcmp(name, "pubDate") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			struct tm t = {0};
			if(strptime(data->value, "%a, %d %b %Y %H:%M:%S %z", &t) != NULL) {
				data->bucket->pubdate = mktime(&t);
			} else {
				fprintf(stderr, "strptime FAILEEEEEEEEEEEEEED\n");
			}
		}
		/*else {*/
			/*write_feed_element(data->feed_path, PUBDATE_FILE, data->value, sizeof(char) * (data->value_len + 1));*/
		/*}*/
	} else if (strcmp(name, "guid") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->guid, data->value, sizeof(char) * data->value_len);
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->author, data->value, sizeof(char) * data->value_len);
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->category, data->value, sizeof(char) * data->value_len);
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->comments, data->value, sizeof(char) * data->value_len);
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
}

static void XMLCALL
charData(void *userData, const XML_Char *s, int s_len)
{
	struct feed_parser_data *data = userData;
	if (data->prev_pos != data->pos) data->value_len = 0;
	if (data->value_len + s_len + 1 > data->value_lim) {
		data->value_lim = data->value_len + s_len + 1;
		data->value_lim *= 2; // just to minimize number of further realloc calls
		data->value = realloc(data->value, data->value_lim);
		if (data->value == NULL) return;
	}
	memcpy(data->value + data->value_len, s, s_len);
	data->value_len += s_len;
	data->value[data->value_len] = '\0';
	data->prev_pos = data->pos;
}


int
parse_rss20(XML_Parser *parser, char *feed_url)
{
	int error = 0;
	struct item_bucket new_bucket = {0};
	struct feed_parser_data feed_data = {
		.value     = malloc(sizeof(char) * INIT_PARSER_BUF_SIZE),
		.value_len = 0,
		.value_lim = INIT_PARSER_BUF_SIZE,
		.depth     = 0,
		.pos       = IN_ROOT,
		.prev_pos  = IN_ROOT,
		.feed_url  = feed_url,
		.bucket    = &new_bucket
	};
	XML_SetUserData(*parser, &feed_data);
	XML_SetElementHandler(*parser, &startElement, &endElement);
	XML_SetCharacterDataHandler(*parser, &charData);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		error = 1;
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
	}
	free(feed_data.value);
	return error;
}
