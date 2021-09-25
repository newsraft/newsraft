#include <string.h>
#include "feedeater.h"

void XMLCALL
parse_atom03_element_beginning(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct parser_data *data = userData;
	++(data->depth);

	if      (strcmp(name, "entry") == 0)    data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)    data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "summary") == 0)  data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)     {
		data->pos |= IN_LINK_ELEMENT;
		const char *href_link = get_value_of_attribute_key(atts, "href");
		if (href_link != NULL) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0)
				cpy_string_array(data->bucket->url, href_link, strlen(href_link));
			else
				db_update_feed_text(data->feed_url, "resource", href_link, strlen(href_link));
		}
	}
	else if (strcmp(name, "id") == 0)       data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "issued") == 0)   data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "modified") == 0) data->pos |= IN_UPDDATE_ELEMENT;
	else if (strcmp(name, "author") == 0)    {
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
	else if (strcmp(name, "name") == 0)     data->pos |= IN_NAME_ELEMENT;
	else if (strcmp(name, "url") == 0)      data->pos |= IN_URL_ELEMENT;
	else if (strcmp(name, "email") == 0)    data->pos |= IN_EMAIL_ELEMENT;
	else if (strcmp(name, "category") == 0) data->pos |= IN_CATEGORY_ELEMENT;
}

void XMLCALL
parse_atom03_element_end(void *userData, const XML_Char *name)
{
	struct parser_data *data = userData;
	--(data->depth);
	strip_whitespace_from_edges(data->value, &data->value_len);

	if (strcmp(name, "entry") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		drop_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->title, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "summary") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->content, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
	} else if (strcmp(name, "id") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->guid, data->value, data->value_len);
	} else if (strcmp(name, "issued") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			time_t rawtime = parse_date_rfc3339(data->value, data->value_len);
			if (rawtime != 0) data->bucket->pubdate = rawtime;
		}
	} else if (strcmp(name, "modified") == 0) {
		data->pos &= ~IN_UPDDATE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			time_t rawtime = parse_date_rfc3339(data->value, data->value_len);
			if (rawtime != 0) data->bucket->upddate = rawtime;
		}
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
	} else if (strcmp(name, "name") == 0) {
		data->pos &= ~IN_NAME_ELEMENT;
		if ((data->pos & IN_AUTHOR_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) == 0) {
				/* global author, todo */
			} else {
				if ((data->bucket->authors != NULL) && (data->bucket->authors_count != 0)) {
					data->bucket->authors[data->bucket->authors_count - 1].name = create_string(data->value, data->value_len);
				}
			}
		}
	} else if (strcmp(name, "url") == 0) {
		data->pos &= ~IN_URL_ELEMENT;
		if ((data->pos & IN_AUTHOR_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) == 0) {
				/* global author, todo */
			} else {
				if ((data->bucket->authors != NULL) && (data->bucket->authors_count != 0)) {
					data->bucket->authors[data->bucket->authors_count - 1].link = create_string(data->value, data->value_len);
				}
			}
		}
	} else if (strcmp(name, "email") == 0) {
		data->pos &= ~IN_EMAIL_ELEMENT;
		if ((data->pos & IN_AUTHOR_ELEMENT) != 0) {
			if ((data->pos & IN_ITEM_ELEMENT) == 0) {
				/* global author, todo */
			} else {
				if ((data->bucket->authors != NULL) && (data->bucket->authors_count != 0)) {
					data->bucket->authors[data->bucket->authors_count - 1].email = create_string(data->value, data->value_len);
				}
			}
		}
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) == 0) {
			/* global category, todo */
		} else {
			add_category_to_item_bucket(data->bucket, data->value, data->value_len);
		}
	}
}
