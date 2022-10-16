#include <string.h>
#include "update_feed/parse_xml/parse_xml_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static int8_t
link_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "href");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		return PARSE_OKAY; // Ignore links to feed itself.
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if (data->path[data->depth] == GENERIC_ITEM) {
			if (crtas_or_cpyas(&data->feed.item->url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth] == GENERIC_FEED) {
			if (crtas_or_cpyas(&data->feed.url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.item->attachments, attrs, "type", "type", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.item->attachments, attrs, "length", "size", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.item->content, attrs, "type", "type", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_string(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
published_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->publication_date = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
updated_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->update_date = parse_date_rfc3339(data->text->ptr, data->text->len);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		data->feed.update_date = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
author_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
contributor_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
name_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_AUTHOR) {
		if (data->path[data->depth - 1] == GENERIC_ITEM) {
			if (serialize_string(&data->feed.item->persons, "name", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == GENERIC_FEED) {
			if (serialize_string(&data->feed.persons, "name", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
uri_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_AUTHOR) {
		if (data->path[data->depth - 1] == GENERIC_ITEM) {
			if (serialize_string(&data->feed.item->persons, "url", 3, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == GENERIC_FEED) {
			if (serialize_string(&data->feed.persons, "url", 3, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
email_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_AUTHOR) {
		if (data->path[data->depth - 1] == GENERIC_ITEM) {
			if (serialize_string(&data->feed.item->persons, "email", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == GENERIC_FEED) {
			if (serialize_string(&data->feed.persons, "email", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
category_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	struct string **target;
	if (data->path[data->depth] == GENERIC_ITEM) {
		target = &data->feed.item->extras;
	} else if (data->path[data->depth] == GENERIC_FEED) {
		target = &data->feed.extras;
	} else {
		return PARSE_OKAY; // Ignore misplaced categories.
	}
	const char *attr = get_value_of_attribute_key(attrs, "label");
	if (attr == NULL) {
		attr = get_value_of_attribute_key(attrs, "term");
		if (attr == NULL) {
			return PARSE_OKAY; // Ignore empty categories.
		}
	}
	const size_t attr_len = strlen(attr);
	if (attr_len != 0) {
		if (serialize_caret(target) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(target, "category", 8, attr, attr_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.content, attrs, "type", "type", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_string(&data->feed.content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_atom10_handlers[] = {
	{"entry",       GENERIC_ITEM,    &generic_item_starter, &generic_item_ender},
	{"id",          XML_UNKNOWN_POS, NULL,                  &generic_guid_end},
	{"title",       XML_UNKNOWN_POS, NULL,                  &generic_title_end},
	{"link",        XML_UNKNOWN_POS, &link_start,           NULL},
	{"summary",     XML_UNKNOWN_POS, &content_start,        &content_end},
	{"content",     XML_UNKNOWN_POS, &content_start,        &content_end},
	{"published",   XML_UNKNOWN_POS, NULL,                  &published_end},
	{"updated",     XML_UNKNOWN_POS, NULL,                  &updated_end},
	{"author",      ATOM10_AUTHOR,   &author_start,         NULL},
	{"contributor", ATOM10_AUTHOR,   &contributor_start,    NULL},
	{"name",        XML_UNKNOWN_POS, NULL,                  &name_end},
	{"uri",         XML_UNKNOWN_POS, NULL,                  &uri_end},
	{"email",       XML_UNKNOWN_POS, NULL,                  &email_end},
	{"category",    XML_UNKNOWN_POS, &category_start,       NULL},
	{"subtitle",    XML_UNKNOWN_POS, &subtitle_start,       &subtitle_end},
	{"generator",   XML_UNKNOWN_POS, &generator_start,      &generator_end},
	{"feed",        GENERIC_FEED,    NULL,                  NULL},
	{NULL,          XML_UNKNOWN_POS, NULL,                  NULL},
};
