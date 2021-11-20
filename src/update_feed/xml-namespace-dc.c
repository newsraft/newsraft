#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

void XMLCALL
parse_dc_element_beginning(void *userData, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	struct parser_data *data = userData;
	++(data->depth);

	     if (strcmp(name, "title") == 0)       data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "creator") == 0)    {
		data->pos |= IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) == 0) {
			/* feed can have global author, todo */
		} else {
			++(data->bucket->authors_count);
			data->bucket->authors = realloc(data->bucket->authors, sizeof(struct author) * data->bucket->authors_count);
			if (data->bucket->authors != NULL) {
				data->bucket->authors[data->bucket->authors_count - 1].name = NULL;
				data->bucket->authors[data->bucket->authors_count - 1].link = NULL;
				data->bucket->authors[data->bucket->authors_count - 1].email = NULL;
			}
		}
	}
}

void XMLCALL
parse_dc_element_end(void *userData, const XML_Char *name)
{
	struct parser_data *data = userData;
	--(data->depth);
	strip_whitespace_from_edges(data->value, &data->value_len);

	if (strcmp(name, "title") == 0) {
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
	} else if (strcmp(name, "creator") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) == 0) {
			/* feed can have global author, todo */
		} else {
			if ((data->bucket->authors != NULL) && (data->bucket->authors_count != 0)) {
				data->bucket->authors[data->bucket->authors_count - 1].name = create_string(data->value, data->value_len);
			}
		}
	}
}
