#include <string.h>
#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://web.archive.org/web/20210303084351/https://pythonhosted.org/feedparser/annotated-atom03.html
// https://web.archive.org/web/20210417183310/http://rakaz.nl/2005/07/moving-from-atom-03-to-10.html
// https://web.archive.org/web/20220502094905/https://support.google.com/merchants/answer/160598?hl=en

// Note to the future.
// Atom 0.3 does not have category element.

static int8_t
entry_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (prepend_item(&data->feed.item) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
id_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (copy_type_of_text_construct(&data->feed.item->title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM03_FEED) {
		if (copy_type_of_text_construct(&data->feed.title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM03_FEED) {
		if (crtss_or_cpyss(&data->feed.title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

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
		if (data->path[data->depth] == ATOM03_ENTRY) {
			if (crtas_or_cpyas(&data->feed.item->url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth] == ATOM03_FEED) {
			if (crtas_or_cpyas(&data->feed.url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else if (data->path[data->depth] == ATOM03_ENTRY) {
		if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.item->attachments, attrs, "type", "type", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		// Atom 0.3 link element doesn't have length and hreflang attributes.
	}
	return PARSE_OKAY;
}

static int8_t
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
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
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
issued_end(struct stream_callback_data *data)
{
	// Atom 0.3 feed can have issued date but who needs it?
	if (data->path[data->depth] == ATOM03_ENTRY) {
		data->feed.item->pubdate = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
modified_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_ENTRY) {
		data->feed.item->upddate = parse_date_rfc3339(data->text->ptr, data->text->len);
	} else if (data->path[data->depth] == ATOM03_FEED) {
		data->feed.update_date = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
author_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (cat_caret_to_serialization(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM03_FEED) {
		if (cat_caret_to_serialization(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
contributor_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == ATOM03_ENTRY) {
		if (cat_caret_to_serialization(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM03_FEED) {
		if (cat_caret_to_serialization(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
name_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_AUTHOR) {
		if (data->path[data->depth - 1] == ATOM03_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->persons, "name", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM03_FEED) {
			if (cat_string_to_serialization(&data->feed.persons, "name", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
url_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_AUTHOR) {
		if (data->path[data->depth - 1] == ATOM03_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->persons, "url", 3, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM03_FEED) {
			if (cat_string_to_serialization(&data->feed.persons, "url", 3, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
email_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_AUTHOR) {
		if (data->path[data->depth - 1] == ATOM03_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->persons, "email", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM03_FEED) {
			if (cat_string_to_serialization(&data->feed.persons, "email", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
tagline_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM03_FEED) {
		if (cat_caret_to_serialization(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_attribute(&data->feed.content, attrs, "type", "type", 4) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
tagline_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM03_FEED) {
		if (cat_string_to_serialization(&data->feed.content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_atom03_handlers[] = {
	{"entry",       ATOM03_ENTRY,     &entry_start,       NULL},
	{"id",          ATOM03_ID,        NULL,               &id_end},
	{"title",       ATOM03_TITLE,     &title_start,       &title_end},
	{"link",        XML_UNKNOWN_POS,  &link_start,        NULL},
	{"summary",     ATOM03_SUMMARY,   &content_start,     &content_end},
	{"content",     ATOM03_CONTENT,   &content_start,     &content_end},
	{"issued",      ATOM03_ISSUED,    NULL,               &issued_end},
	{"modified",    ATOM03_MODIFIED,  NULL,               &modified_end},
	{"author",      ATOM03_AUTHOR,    &author_start,      NULL},
	{"contributor", ATOM03_AUTHOR,    &contributor_start, NULL},
	{"name",        ATOM03_NAME,      NULL,               &name_end},
	{"url",         ATOM03_URL,       NULL,               &url_end},
	{"email",       ATOM03_EMAIL,     NULL,               &email_end},
	{"tagline",     ATOM03_TAGLINE,   &tagline_start,     &tagline_end},
	{"generator",   ATOM03_GENERATOR, &generator_start,   &generator_end},
	{"feed",        ATOM03_FEED,      NULL,               NULL},
	{NULL,          XML_UNKNOWN_POS,  NULL,               NULL},
};
