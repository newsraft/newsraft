#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "feedeater.h"
#include "config.h"

static enum xml_pos last_pos = 0;
static char value[1000000];
static int value_len = 0;

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct feed_parser_data *data = userData;
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
	data->depth += 1;
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	struct feed_parser_data *data = userData;
	/*(void)name;*/
	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_bucket(data->bucket, data->feed_url);
		free_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				malstrcpy(&data->bucket->title, value, sizeof(char) * (value_len + 1));
			}
			/*else {*/
				/*write_feed_element(data->feed_path, TITLE_FILE, value, sizeof(char) * (value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				malstrcpy(&data->bucket->content, value, sizeof(char) * (value_len + 1));
			}
			/*else {*/
				/*write_feed_element(data->feed_path, CONTENT_FILE, value, sizeof(char) * (value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				malstrcpy(&data->bucket->link, value, sizeof(char) * (value_len + 1));
			}
			/*else {*/
				/*write_feed_element(data->feed_path, LINK_FILE, value, sizeof(char) * (value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "pubDate") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				malstrcpy(&data->bucket->pubdate, value, sizeof(char) * (value_len + 1));
			}
			/*else {*/
				/*write_feed_element(data->feed_path, PUBDATE_FILE, value, sizeof(char) * (value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "guid") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			malstrcpy(&data->bucket->uid, value, sizeof(char) * (value_len + 1));
		}
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			malstrcpy(&data->bucket->category, value, sizeof(char) * (value_len + 1));
		}
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			malstrcpy(&data->bucket->comments, value, sizeof(char) * (value_len + 1));
		}
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			malstrcpy(&data->bucket->author, value, sizeof(char) * (value_len + 1));
		}
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
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


int
parse_rss20(XML_Parser *parser, char *feed_url)
{
	struct item_bucket *bucket = calloc(1, sizeof(struct item_bucket));
	struct feed_parser_data feed_data = {0, IN_ROOT, feed_url, bucket};

	XML_SetUserData(*parser, &feed_data);
	XML_SetElementHandler(*parser, startElement, endElement);
	XML_SetCharacterDataHandler(*parser, charData);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
		return 0;
	}

	free(bucket);
	return 1;
}
