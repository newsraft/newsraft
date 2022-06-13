#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

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
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM10_FEED) {
		if (crtss_or_cpyss(&data->feed.guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (copy_type_of_text_construct(&data->feed.item->title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM10_FEED) {
		if (copy_type_of_text_construct(&data->feed.title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM10_FEED) {
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
	size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		return PARSE_OKAY; // Ignore links to feed itself.
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if (data->path[data->depth] == ATOM10_ENTRY) {
			if (crtas_or_cpyas(&data->feed.item->url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth] == ATOM10_FEED) {
			if (crtas_or_cpyas(&data->feed.url, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else if (data->path[data->depth] == ATOM10_ENTRY) {
		empty_link(&data->feed.temp);
		if (crtas_or_cpyas(&data->feed.temp.attachment.url, attr, attr_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		attr = get_value_of_attribute_key(attrs, "type");
		if (attr != NULL) {
			attr_len = strlen(attr);
			if (attr_len != 0) {
				if (crtas_or_cpyas(&data->feed.temp.attachment.type, attr, attr_len) == false) {
					return PARSE_FAIL_NOT_ENOUGH_MEMORY;
				}
			}
		}
		attr = get_value_of_attribute_key(attrs, "length");
		if (attr != NULL) {
			data->feed.temp.attachment.size = convert_string_to_size_t_or_zero(attr);
		}
		if (serialize_link(&data->feed.temp, &data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
summary_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (copy_type_of_text_construct(&data->feed.item->summary.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
summary_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->summary.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (copy_type_of_text_construct(&data->feed.item->content.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
published_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		data->feed.item->pubdate = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
updated_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_ENTRY) {
		data->feed.item->upddate = parse_date_rfc3339(data->text->ptr, data->text->len);
	} else if (data->path[data->depth] == ATOM10_FEED) {
		data->feed.update_date = parse_date_rfc3339(data->text->ptr, data->text->len);
	}
	return PARSE_OKAY;
}

static int8_t
author_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (cat_array_to_serialization(&data->feed.item->authors, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM10_FEED) {
		if (cat_array_to_serialization(&data->feed.authors, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
contributor_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == ATOM10_ENTRY) {
		if (cat_array_to_serialization(&data->feed.item->authors, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == ATOM10_FEED) {
		if (cat_array_to_serialization(&data->feed.authors, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
name_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_AUTHOR) {
		if (data->path[data->depth - 1] == ATOM10_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->authors, "name", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM10_FEED) {
			if (cat_string_to_serialization(&data->feed.authors, "name", 4, data->text) == false) {
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
		if (data->path[data->depth - 1] == ATOM10_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->authors, "url", 3, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM10_FEED) {
			if (cat_string_to_serialization(&data->feed.authors, "url", 3, data->text) == false) {
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
		if (data->path[data->depth - 1] == ATOM10_ENTRY) {
			if (cat_string_to_serialization(&data->feed.item->authors, "email", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if (data->path[data->depth - 1] == ATOM10_FEED) {
			if (cat_string_to_serialization(&data->feed.authors, "email", 5, data->text) == false) {
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
	if (data->path[data->depth] == ATOM10_ENTRY) {
		target = &data->feed.item->categories;
	} else if (data->path[data->depth] == ATOM10_FEED) {
		target = &data->feed.categories;
	} else {
		return PARSE_OKAY; // Ignore misplaced categories.
	}
	const char *attr = get_value_of_attribute_key(attrs, "term");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty categories.
	}
	const size_t term_len = strlen(attr);
	if (term_len == 0) {
		return PARSE_OKAY; // Ignore empty categories.
	}
	if (cat_array_to_serialization(target, "term", 4, attr, term_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	attr = get_value_of_attribute_key(attrs, "scheme");
	if (attr != NULL) {
		if (cat_array_to_serialization(target, "scheme", 6, attr, strlen(attr)) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	attr = get_value_of_attribute_key(attrs, "label");
	if (attr != NULL) {
		if (cat_array_to_serialization(target, "label", 5, attr, strlen(attr)) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM10_FEED) {
		if (copy_type_of_text_construct(&data->feed.summary.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_FEED) {
		if (crtss_or_cpyss(&data->feed.summary.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
generator_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == ATOM10_FEED) {
		const char *attr = get_value_of_attribute_key(attrs, "version");
		size_t attr_len;
		if (attr != NULL) {
			attr_len = strlen(attr);
			if (attr_len != 0) {
				if (crtas_or_cpyas(&data->feed.generator.version, attr, attr_len) == false) {
					return PARSE_FAIL_NOT_ENOUGH_MEMORY;
				}
			}
		}
		attr = get_value_of_attribute_key(attrs, "uri");
		if (attr != NULL) {
			attr_len = strlen(attr);
			if (attr_len != 0) {
				if (crtas_or_cpyas(&data->feed.generator.url, attr, attr_len) == false) {
					return PARSE_FAIL_NOT_ENOUGH_MEMORY;
				}
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
generator_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == ATOM10_FEED) {
		if (crtss_or_cpyss(&data->feed.generator.name, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_atom10_handlers[] = {
	{"entry",       ATOM10_ENTRY,     &entry_start,       NULL},
	{"id",          ATOM10_ID,        NULL,               &id_end},
	{"title",       ATOM10_TITLE,     &title_start,       &title_end},
	{"link",        XML_UNKNOWN_POS,  &link_start,        NULL},
	{"summary",     ATOM10_SUMMARY,   &summary_start,     &summary_end},
	{"content",     ATOM10_CONTENT,   &content_start,     &content_end},
	{"published",   ATOM10_PUBLISHED, NULL,               &published_end},
	{"updated",     ATOM10_UPDATED,   NULL,               &updated_end},
	{"author",      ATOM10_AUTHOR,    &author_start,      NULL},
	{"contributor", ATOM10_AUTHOR,    &contributor_start, NULL},
	{"name",        ATOM10_NAME,      NULL,               &name_end},
	{"uri",         ATOM10_URI,       NULL,               &uri_end},
	{"email",       ATOM10_EMAIL,     NULL,               &email_end},
	{"category",    XML_UNKNOWN_POS,  &category_start,    NULL},
	{"subtitle",    ATOM10_SUBTITLE,  &subtitle_start,    &subtitle_end},
	{"generator",   ATOM10_GENERATOR, &generator_start,   &generator_end},
	{"feed",        ATOM10_FEED,      NULL,               NULL},
	{NULL,          XML_UNKNOWN_POS,  NULL,               NULL},
};
