#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

int16_t dc_pos;

void XMLCALL
parse_dc_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;

	     if (strcmp(name, "title") == 0)       data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "creator") == 0)    {
		data->pos |= IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			if (expand_authors_of_item_bucket_by_one_element(data->bucket) != 0) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			/* feed can have global author, todo */
		}
	}
}

void XMLCALL
parse_dc_element_end(struct parser_data *data, const XML_Char *name)
{
	if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->title, data->value, data->value_len);
		else if ((data->pos & IN_CHANNEL_ELEMENT) != 0)
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->content, data->value, data->value_len);
		else if ((data->pos & IN_CHANNEL_ELEMENT) != 0)
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "creator") == 0) {
		if ((data->pos & IN_AUTHOR_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) == 0) {
				/* global author, todo */
			} else {
				if (add_name_to_last_author_of_item_bucket(data->bucket, data->value, data->value_len) != 0) {
					data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
					return;
				}
			}
		}
		data->pos &= ~IN_AUTHOR_ELEMENT;
	}
}
#endif // FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
