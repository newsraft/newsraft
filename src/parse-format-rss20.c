#include <string.h>
#include "feedeater.h"

static void XMLCALL
process_element_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct feed_parser_data *data = userData;
	++(data->depth);

	if (process_namespaced_tag_start(userData, name, atts) == 0) {
		return;
	}

	if      (strcmp(name, "item") == 0)          data->pos |= IN_ITEM_ELEMENT;
	else if (strcmp(name, "title") == 0)         data->pos |= IN_TITLE_ELEMENT;
	else if (strcmp(name, "description") == 0)   data->pos |= IN_DESCRIPTION_ELEMENT;
	else if (strcmp(name, "link") == 0)          data->pos |= IN_LINK_ELEMENT;
	else if (strcmp(name, "pubDate") == 0)       data->pos |= IN_PUBDATE_ELEMENT;
	else if (strcmp(name, "guid") == 0)          data->pos |= IN_GUID_ELEMENT;
	else if (strcmp(name, "author") == 0)        data->pos |= IN_AUTHOR_ELEMENT;
	else if (strcmp(name, "category") == 0)      data->pos |= IN_CATEGORY_ELEMENT;
	else if (strcmp(name, "comments") == 0)      data->pos |= IN_COMMENTS_ELEMENT;
	else if (strcmp(name, "channel") == 0)       data->pos |= IN_CHANNEL_ELEMENT;
}

static void XMLCALL
process_element_end(void *userData, const XML_Char *name)
{
	struct feed_parser_data *data = userData;
	--(data->depth);
	value_strip_whitespace(data->value, &data->value_len);

	if (process_namespaced_tag_end(userData, name) == 0) {
		return;
	}

	if ((data->pos & IN_CHANNEL_ELEMENT) == 0) return;
	if (strcmp(name, "item") == 0) {
		data->pos &= ~IN_ITEM_ELEMENT;
		try_item_bucket(data->bucket, data->feed_url);
		drop_item_bucket(data->bucket);
	} else if (strcmp(name, "title") == 0) {
		data->pos &= ~IN_TITLE_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->title, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	} else if (strcmp(name, "description") == 0) {
		data->pos &= ~IN_DESCRIPTION_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->content, data->value, data->value_len);
		else
			db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	} else if (strcmp(name, "link") == 0) {
		data->pos &= ~IN_LINK_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->url, data->value, data->value_len);
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
			cpy_string_array(data->bucket->guid, data->value, data->value_len);
	} else if (strcmp(name, "author") == 0) {
		data->pos &= ~IN_AUTHOR_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->author, data->value, data->value_len);
	} else if (strcmp(name, "category") == 0) {
		data->pos &= ~IN_CATEGORY_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0) {
			if (data->bucket->category->len == 0) {
				cpy_string_array(data->bucket->category, data->value, data->value_len);
			} else if (data->value_len != 0) {
				cat_string_array(data->bucket->category, ", ", 2);
				cat_string_array(data->bucket->category, data->value, data->value_len);
			}
		}
	} else if (strcmp(name, "comments") == 0) {
		data->pos &= ~IN_COMMENTS_ELEMENT;
		if ((data->pos & IN_ITEM_ELEMENT) != 0)
			cpy_string_array(data->bucket->comments, data->value, data->value_len);
	} else if (strcmp(name, "channel") == 0) {
		data->pos &= ~IN_CHANNEL_ELEMENT;
	}
}

int
parse_rss20(XML_Parser *parser)
{
	int error = 0;
	XML_SetElementHandler(*parser, &process_element_start, &process_element_end);
	XML_SetCharacterDataHandler(*parser, &store_xml_element_value);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		debug_write(DBG_ERR, "parse_rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",
		            XML_ErrorString(XML_GetErrorCode(*parser)),
		            XML_GetCurrentLineNumber(*parser));
		error = 1;
	}
	return error;
}
