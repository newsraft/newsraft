// http://purl.org/rss/1.0/
// http://channel.netscape.com/rdf/simple/0.9/
#include <string.h>
#include "feedeater.h"

void XMLCALL
rss_10_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	struct feed_parser_data *data = userData;

	if      (strcmp(name, "item") == 0)        data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)       data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)        data->pos |= IN_LINK_ELEMENT;
	/*else if (strcmp(name, "image") == 0)       data->pos |= IN_IMAGE_ELEMENT;*/
	else if (strcmp(name, "channel") == 0)     data->pos |= IN_CHANNEL_ELEMENT;
}

void XMLCALL
rss_10_end(void *userData, const XML_Char *name)
{
	struct feed_parser_data *data = userData;

	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		drop_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->title, data->value, data->value_len);
		else if ((data->pos & IN_CHANNEL_ELEMENT) != 0)
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->content, data->value, data->value_len);
		else if ((data->pos & IN_CHANNEL_ELEMENT) != 0)
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->url, data->value, data->value_len);
		else if ((data->pos & IN_CHANNEL_ELEMENT) != 0)
			db_update_feed_text(data->feed_url, "url", data->value, data->value_len);
	/*} else if (strcmp(name, "image") == 0) {*/
		/*data->pos &= ~IN_IMAGE_ELEMENT;*/
		/*if ((data->pos & IN_ITEM_ELEMENT) != 0)*/
			/*cpy_string_array(data->bucket->url, data->value, data->value_len);*/
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
}
