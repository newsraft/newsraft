#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static enum xml_pos last_pos = 0;
static char value[100000];
static int value_len = 0;

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct feed_parser_data *data = userData;
	if (strcmp(name, "item") == 0) {
		data->pos |= IN_ITEM_ELEMENT;
		if (data->items_count <= MAX_ITEMS) {
			++(data->items_count);
			data->item_path = feed_item_data_path(data->feed->feed_url, data->items_count);
		}
	} else if (strcmp(name, "title") == 0)     data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)        data->pos |= IN_LINK_ELEMENT;
	else if (strcmp(name, "author") == 0)      data->pos |= IN_AUTHOR_ELEMENT;
	else if (strcmp(name, "channel") == 0)     data->pos |= IN_CHANNEL_ELEMENT;

	data->depth += 1;
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	struct feed_parser_data *data = userData;
	/*(void)name;*/
	if        (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		if (data->items_count <= MAX_ITEMS) {
			if (data->item_path != NULL) free(data->item_path);
		}
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && (value_len != 0)) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				if (data->items_count < MAX_ITEMS) {
					write_feed_item_elem(data->item_path, "title", value, sizeof(char) * value_len);
				}
			} else {
				data->feed->rname = malloc(sizeof(char) * (value_len + 1));
				strcpy(data->feed->rname, value);
			}
		}
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && (value_len != 0)) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				if (data->items_count < MAX_ITEMS) {
					write_feed_item_elem(data->item_path, "description", value, sizeof(char) * value_len);
				}
			}
			/*else {*/
			/*}*/
		}
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) == 0)) {
			if (strlen(value) != 0) {
				data->feed->site_url = malloc(sizeof(char) * (value_len + 1));
				strcpy(data->feed->site_url, value);
			}
		}
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) &&
		    ((data->pos & IN_ITEM_ELEMENT) != 0) &&
		    (data->items_count < MAX_ITEMS) &&
		    (value_len != 0))
		{
			write_feed_item_elem(data->item_path, "author", value, sizeof(char) * value_len);
		}
	} else if (strcmp(name, "channel") == 0) data->pos &= ~IN_CHANNEL_ELEMENT;

	data->depth -= 1;
}

static void XMLCALL
charData(void *userData, const XML_Char *s, int len)
{
	struct feed_parser_data *data = userData;
	if (last_pos != data->pos) {
		value_len = 0;
	}
	memcpy(value + value_len, s, len);
	value_len += len;
	value[value_len] = '\0';
	last_pos = data->pos;
}


struct feed_entry *
parse_rss20(XML_Parser *parser, char *url)
{
	/*status_write("rss20 beign parsed");*/
	/*return NULL;*/
	struct feed_entry *feed = calloc(1, sizeof(struct feed_entry));
	if (feed == NULL) {
		status_write("calloc on feed buffer failed\n"); return NULL;
	}
	feed->feed_url = malloc(sizeof(char) * (strlen(url) + 1));
	if (feed->feed_url == NULL) {
		free(feed);
		status_write("malloc on feed url failed\n");
		return NULL;
	}
	strcpy(feed->feed_url, url);

	struct feed_parser_data feed_data = {0, 0, feed, 0, NULL};

	XML_SetUserData(*parser, &feed_data);
	XML_SetElementHandler(*parser, startElement, endElement);
	XML_SetCharacterDataHandler(*parser, charData);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		free(feed);
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
		return NULL;
	}

	return feed;
}
