#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

void XMLCALL
parse_rss20_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	if (parse_namespace_element_start(data, name, atts) == 0) {
		return;
	}

	     if (strcmp(name, "item") == 0)        data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)       data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "link") == 0)        data->pos |= IN_LINK_ELEMENT;
	else if (strcmp(name, "description") == 0) data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "pubDate") == 0)     data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "guid") == 0)        data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "author") == 0)      {
		data->pos |= IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			++(data->bucket->authors_count);
			data->bucket->authors = realloc(data->bucket->authors, sizeof(struct author) * data->bucket->authors_count);
			if (data->bucket->authors != NULL) {
				data->bucket->authors[data->bucket->authors_count - 1].name = NULL;
				data->bucket->authors[data->bucket->authors_count - 1].link = NULL;
				data->bucket->authors[data->bucket->authors_count - 1].email = NULL;
			}
		}
	}
	else if (strcmp(name, "category") == 0)    data->pos |= IN_CATEGORY_ELEMENT;
	else if (strcmp(name, "comments") == 0)    data->pos |= IN_COMMENTS_ELEMENT;
	else if (strcmp(name, "channel") == 0)     data->pos |= IN_CHANNEL_ELEMENT;
}

void XMLCALL
parse_rss20_element_end(struct parser_data *data, const XML_Char *name)
{
	if (parse_namespace_element_end(data, name) == 0) {
		return;
	}

	strip_whitespace_from_edges(data->value, &data->value_len);

	if ((data->pos & IN_CHANNEL_ELEMENT) == 0) return;

	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		drop_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->title, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->content, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->url, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "resource", data->value, data->value_len);
	} else if (strcmp(name, "pubDate") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			time_t rawtime = parse_date_rfc822(data->value, data->value_len);
			if (rawtime != 0) data->bucket->pubdate = rawtime;
		}
	} else if (strcmp(name, "guid") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->guid, data->value, data->value_len);
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			if ((data->bucket->authors != NULL) && (data->bucket->authors_count != 0)) {
				data->bucket->authors[data->bucket->authors_count - 1].email = create_string(data->value, data->value_len);
			}
		}
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			add_category_to_item_bucket(data->bucket, data->value, data->value_len);
		} else {
			/* global category, todo */
		}
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpyas(data->bucket->comments, data->value, data->value_len);
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
}
