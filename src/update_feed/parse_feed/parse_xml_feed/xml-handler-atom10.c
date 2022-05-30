#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
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
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.guid, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (copy_type_of_text_construct(&data->feed.item->title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (copy_type_of_text_construct(&data->feed.title.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.title.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
link_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	const char *href = get_value_of_attribute_key(attrs, "href");
	if (href == NULL) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const size_t href_len = strlen(href);
	if (href_len == 0) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		return PARSE_OKAY; // Ignore links to feed itself.
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
			if (crtas_or_cpyas(&data->feed.item->url, href, href_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (crtas_or_cpyas(&data->feed.url, href, href_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_link(&data->feed.item->attachment) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->attachment->url, href, href_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		const char *type = get_value_of_attribute_key(attrs, "type");
		if (type != NULL) {
			if (crtas_or_cpyas(&data->feed.item->attachment->type, type, strlen(type)) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
		const char *length = get_value_of_attribute_key(attrs, "length");
		if (length != NULL) {
			data->feed.item->attachment->size = convert_string_to_size_t_or_zero(length);
		}
	}
	return PARSE_OKAY;
}

static int8_t
summary_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (copy_type_of_text_construct(&data->feed.item->summary.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
summary_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->summary.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (copy_type_of_text_construct(&data->feed.item->content.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->content.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
published_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		data->feed.item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
	return PARSE_OKAY;
}

static int8_t
updated_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		data->feed.item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed.update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
	return PARSE_OKAY;
}

static int8_t
author_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_person(&data->feed.item->author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (prepend_person(&data->feed.author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
name_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) != 0) {
		if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
			if (crtss_or_cpyss(&data->feed.item->author->name, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (crtss_or_cpyss(&data->feed.author->name, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
uri_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) != 0) {
		if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
			if (crtss_or_cpyss(&data->feed.item->author->url, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (crtss_or_cpyss(&data->feed.author->url, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
email_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) != 0) {
		if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
			if (crtss_or_cpyss(&data->feed.item->author->email, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (crtss_or_cpyss(&data->feed.author->email, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
category_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	const char *term = get_value_of_attribute_key(attrs, "term");
	if (term == NULL) {
		return PARSE_OKAY; // Ignore empty categories.
	}
	const size_t term_len = strlen(term);
	if (term_len == 0) {
		return PARSE_OKAY; // Ignore empty categories.
	}
	const char *label = get_value_of_attribute_key(attrs, "label");
	const char *scheme = get_value_of_attribute_key(attrs, "scheme");
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_category(&data->feed.item->category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->category->term, term, term_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (label != NULL) {
			if (crtas_or_cpyas(&data->feed.item->category->label, label, strlen(label)) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
		if (scheme != NULL) {
			if (crtas_or_cpyas(&data->feed.item->category->scheme, scheme, strlen(scheme)) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if (prepend_category(&data->feed.category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.category->term, term, term_len) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (label != NULL) {
			if (crtas_or_cpyas(&data->feed.category->label, label, strlen(label)) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
		if (scheme != NULL) {
			if (crtas_or_cpyas(&data->feed.category->scheme, scheme, strlen(scheme)) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		if (copy_type_of_text_construct(&data->feed.summary.type, attrs) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subtitle_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		if (crtss_or_cpyss(&data->feed.summary.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
generator_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
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
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		if (crtss_or_cpyss(&data->feed.generator.name, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_atom10_handlers[] = {
	{"entry",       ATOM10_ENTRY,     &entry_start,     NULL},
	{"id",          ATOM10_ID,        NULL,             &id_end},
	{"title",       ATOM10_TITLE,     &title_start,     &title_end},
	{"link",        ATOM10_NONE,      &link_start,      NULL},
	{"summary",     ATOM10_SUMMARY,   &summary_start,   &summary_end},
	{"content",     ATOM10_CONTENT,   &content_start,   &content_end},
	{"published",   ATOM10_PUBLISHED, NULL,             &published_end},
	{"updated",     ATOM10_UPDATED,   NULL,             &updated_end},
	{"author",      ATOM10_AUTHOR,    &author_start,    NULL},
	{"contributor", ATOM10_AUTHOR,    &author_start,    NULL},
	{"name",        ATOM10_NAME,      NULL,             &name_end},
	{"uri",         ATOM10_URI,       NULL,             &uri_end},
	{"email",       ATOM10_EMAIL,     NULL,             &email_end},
	{"category",    ATOM10_NONE,      &category_start,  NULL},
	{"subtitle",    ATOM10_SUBTITLE,  &subtitle_start,  &subtitle_end},
	{"generator",   ATOM10_GENERATOR, &generator_start, &generator_end},
	{NULL,          ATOM10_NONE,      NULL,             NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_ATOM10
