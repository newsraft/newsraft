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
	IN_LANGUAGE_ELEMENT = 4096,
	IN_LASTBUILDDATE_ELEMENT = 8192,
	IN_CHANNEL_ELEMENT = 16384,
};

static void XMLCALL
process_element_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	/*(void)atts;*/
	struct feed_parser_data *data = userData;
	++(data->depth);
	if      (strcmp(name, "item") == 0)          data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)         data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0)   data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)          data->pos |= IN_LINK_ELEMENT;
	else if (strcmp(name, "pubDate") == 0)       data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "guid") == 0)          data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "category") == 0)      data->pos |= IN_CATEGORY_ELEMENT;
	else if (strcmp(name, "comments") == 0)      data->pos |= IN_COMMENTS_ELEMENT;
	else if (strcmp(name, "author") == 0)        data->pos |= IN_AUTHOR_ELEMENT;
	else if (strcmp(name, "lastBuildDate") == 0) data->pos |= IN_LASTBUILDDATE_ELEMENT;
	else if (strcmp(name, "language") == 0)      data->pos |= IN_LANGUAGE_ELEMENT;
	else if (strcmp(name, "channel") == 0)       data->pos |= IN_CHANNEL_ELEMENT;
}

static void XMLCALL
process_element_finish(void *userData, const XML_Char *name)
{
	/*(void)name;*/
	struct feed_parser_data *data = userData;
	--(data->depth);
	if ((data->pos & IN_CHANNEL_ELEMENT) == 0) return;
	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		reset_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->title, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->content, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->url, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "resource", data->value, data->value_len);
	} else if (strcmp(name, "pubDate") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		time_t rawtime = get_unix_epoch_time("%a, %d %b %Y %H:%M:%S %z", data->value);
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			data->bucket->pubdate = rawtime;
		else
			db_update_feed_int64(data->feed_url, "pubdate", (int64_t)rawtime);
	} else if (strcmp(name, "guid") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->guid, data->value, data->value_len);
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->author, data->value, data->value_len);
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->category, data->value, data->value_len);
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->comments, data->value, data->value_len);
	} else if (strcmp(name, "language") == 0) {
		data->pos &= ~IN_LANGUAGE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) == 0)
			db_update_feed_text(data->feed_url, "language", data->value, data->value_len);
	} else if (strcmp(name, "lastBuildDate") == 0) {
		data->pos &= ~IN_LASTBUILDDATE_ELEMENT;
		time_t rawtime = get_unix_epoch_time("%a, %d %b %Y %H:%M:%S %z", data->value);
		if ((data->pos & IN_ITEM_ELEMENT) == 0)
			db_update_feed_int64(data->feed_url, "builddate", (int64_t)rawtime);
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
}

int
parse_rss20(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data)
{
	int error = 0;
	XML_SetUserData(*parser, feed_data);
	XML_SetElementHandler(*parser, &process_element_start, &process_element_finish);
	XML_SetCharacterDataHandler(*parser, &store_xml_element_value);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		error = 1;
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
	}
	return error;
}
