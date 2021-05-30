// http://www.w3.org/2005/Atom
#include <stdio.h>
#include <string.h>
#include "feedeater.h"

static time_t
parse_atom_10_time(char *value, size_t value_len)
{
	time_t rawtime = 0;
	if (value_len >= 20) {
		if (value[value_len - 1] == 'Z') {
			rawtime = get_unix_epoch_time("%Y-%m-%dT%H:%M:%SZ", value);
		} else {
			rawtime = get_unix_epoch_time("%Y-%m-%dT%H:%M:%S%z", value);
		}
	}
	return rawtime;
}

void XMLCALL
atom_10_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	/*(void)atts;*/
	struct feed_parser_data *data = userData;
	int i;

	if      (strcmp(name, "entry") == 0)    data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)    data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "summary") == 0)  data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)     {
		data->pos |= IN_LINK_ELEMENT;
		for (i = 0; atts[i] != NULL && strcmp(atts[i], "href") != 0; ++i) {};
		if (atts[i] != NULL) ++i;
		if (atts[i] != NULL) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0)
				make_string(&data->bucket->url, (void *)atts[i], strlen(atts[i]));
			else
				db_update_feed_text(data->feed_url, "resource", (void *)atts[i], strlen(atts[i]));
		}
	}
	else if (strcmp(name, "id") == 0)        data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "published") == 0) data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "updated") == 0)   data->pos |= IN_LASTBUILDDATE_ELEMENT;
	else if (strcmp(name, "author") == 0)    data->pos |= IN_AUTHOR_ELEMENT;
	else if (strcmp(name, "category") == 0)  data->pos |= IN_CATEGORY_ELEMENT;
}

void XMLCALL
atom_10_end(void *userData, const XML_Char *name)
{
	/*(void)name;*/
	struct feed_parser_data *data = userData;

	if (strcmp(name, "entry") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		reset_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->title, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "summary") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->content, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
	} else if (strcmp(name, "id") == 0) {
		data->pos &= ~IN_GUID_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->guid, data->value, data->value_len);
	} else if (strcmp(name, "published") == 0) {
		data->pos &= ~IN_PUBDATE_ELEMENT;
		time_t rawtime = parse_atom_10_time(data->value, data->value_len);
		if (rawtime > 0) {
			if ((data->pos & IN_ITEM_ELEMENT) != 0)
				data->bucket->pubdate = rawtime;
			else
				db_update_feed_int64(data->feed_url, "pubdate", (int64_t)rawtime);
		}
	} else if (strcmp(name, "updated") == 0) {
		data->pos &= ~IN_LASTBUILDDATE_ELEMENT;
		time_t rawtime = parse_atom_10_time(data->value, data->value_len);
		if (rawtime > 0) {
			if ((data->pos & IN_ITEM_ELEMENT) == 0)
				db_update_feed_int64(data->feed_url, "builddate", (int64_t)rawtime);
		}
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->author, data->value, data->value_len);
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			make_string(&data->bucket->category, data->value, data->value_len);
	}
}
