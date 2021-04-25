#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "feedeater.h"
#include "config.h"

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
		try_item_bucket(data->bucket, data->feed_url);
		reset_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				make_string(&data->bucket->title, data->value, sizeof(char) * data->value_len);
			}
			/*else {*/
				/*write_feed_element(data->feed_path, TITLE_FILE, data->value, sizeof(char) * (data->value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				make_string(&data->bucket->content, data->value, sizeof(char) * data->value_len);
			}
			/*else {*/
				/*write_feed_element(data->feed_path, CONTENT_FILE, data->value, sizeof(char) * (data->value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0) {
				make_string(&data->bucket->url, data->value, sizeof(char) * data->value_len);
			}
			/*else {*/
				/*write_feed_element(data->feed_path, LINK_FILE, data->value, sizeof(char) * (data->value_len + 1));*/
			/*}*/
		}
	} else if (strcmp(name, "pubDate") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		if ((data->pos & IN_CHANNEL_ELEMENT) != 0) {
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
		}
	} else if (strcmp(name, "guid") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			make_string(&data->bucket->guid, data->value, sizeof(char) * data->value_len);
		}
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			make_string(&data->bucket->author, data->value, sizeof(char) * data->value_len);
		}
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			make_string(&data->bucket->category, data->value, sizeof(char) * data->value_len);
		}
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if (((data->pos & IN_CHANNEL_ELEMENT) != 0) && ((data->pos & IN_ITEM_ELEMENT) != 0)) {
			make_string(&data->bucket->comments, data->value, sizeof(char) * data->value_len);
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
	if (data->last_pos != data->pos) {
		data->value_len = 0;
	}
	memcpy(data->value + data->value_len, s, len);
	data->value_len += len;
	data->value[data->value_len] = '\0';
	data->last_pos = data->pos;
}


int
parse_rss20(XML_Parser *parser, char *feed_url)
{
	char *value = malloc(sizeof(char) * 1040000); // around megabyte
	struct item_bucket bucket = {0};
	struct feed_parser_data feed_data = {value, 0, 0, IN_ROOT, IN_ROOT, feed_url, &bucket};
	XML_SetUserData(*parser, &feed_data);
	XML_SetElementHandler(*parser, &startElement, &endElement);
	XML_SetCharacterDataHandler(*parser, &charData);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
		return 0;
	}
	free(value);
	return 1;
}
