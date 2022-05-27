#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static void
entry_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	prepend_item(&data->feed->item);
}

static void
id_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		return; // Ignore feed id.
	}
	if (crtss_or_cpyss(&data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
title_end(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed->item->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (copy_type_of_text_construct(&data->feed->item->title.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (copy_type_of_text_construct(&data->feed->title.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
link_start(struct xml_data *data, const TidyAttr attrs)
{
	const char *href = get_value_of_attribute_key(attrs, "href");
	if (href == NULL) {
		return; // Ignore empty links.
	}
	const size_t href_len = strlen(href);
	if (href_len == 0) {
		return; // Ignore empty links.
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		return; // Ignore links to feed itself.
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
			if (crtas_or_cpyas(&data->feed->item->url, href, href_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			if (crtas_or_cpyas(&data->feed->url, href, href_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_link(&data->feed->item->attachment) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtas_or_cpyas(&data->feed->item->attachment->url, href, href_len) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		const char *type = get_value_of_attribute_key(attrs, "type");
		if (type != NULL) {
			if (crtas_or_cpyas(&data->feed->item->attachment->type, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		const char *length = get_value_of_attribute_key(attrs, "length");
		if (length != NULL) {
			data->feed->item->attachment->size = convert_string_to_size_t_or_zero(length);
		}
	}
}

static void
summary_end(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed->item->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (copy_type_of_text_construct(&data->feed->item->summary.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
content_end(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) == 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed->item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (copy_type_of_text_construct(&data->feed->item->content.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
published_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		data->feed->item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static void
updated_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		data->feed->item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed->update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static void
author_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_person(&data->feed->item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (prepend_person(&data->feed->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
name_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed->item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
uri_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed->item->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
email_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed->item->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
category_start(struct xml_data *data, const TidyAttr attrs)
{
	const char *term = get_value_of_attribute_key(attrs, "term");
	if (term == NULL) {
		return; // Atom 1.0 says that every category MUST have term attribute.
	}
	const size_t term_len = strlen(term);
	if (term_len == 0) {
		return;
	}
	const char *label = get_value_of_attribute_key(attrs, "label");
	const char *scheme = get_value_of_attribute_key(attrs, "scheme");
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		if (prepend_category(&data->feed->item->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtas_or_cpyas(&data->feed->item->category->term, term, term_len) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (label != NULL) {
			if (crtas_or_cpyas(&data->feed->item->category->label, label, strlen(label)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (scheme != NULL) {
			if (crtas_or_cpyas(&data->feed->item->category->scheme, scheme, strlen(scheme)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (prepend_category(&data->feed->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtas_or_cpyas(&data->feed->category->term, term, term_len) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (label != NULL) {
			if (crtas_or_cpyas(&data->feed->category->label, label, strlen(label)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (scheme != NULL) {
			if (crtas_or_cpyas(&data->feed->category->scheme, scheme, strlen(scheme)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static void
subtitle_end(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (copy_type_of_text_construct(&data->feed->summary.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
generator_end(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		return;
	}
	const char *version = get_value_of_attribute_key(attrs, "version");
	if (version != NULL) {
		if (crtas_or_cpyas(&data->feed->generator.version, version, strlen(version)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *uri = get_value_of_attribute_key(attrs, "uri");
	if (uri != NULL) {
		if (crtas_or_cpyas(&data->feed->generator.url, uri, strlen(uri)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	if (crtss_or_cpyss(&data->feed->generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_atom10_handlers[] = {
	{"entry",       ATOM10_ENTRY,     &entry_start,    NULL},
	{"id",          ATOM10_ID,        NULL,            &id_end},
	{"title",       ATOM10_TITLE,     NULL,            &title_end},
	{"link",        ATOM10_NONE,      &link_start,     NULL},
	{"summary",     ATOM10_SUMMARY,   NULL,            &summary_end},
	{"content",     ATOM10_CONTENT,   NULL,            &content_end},
	{"published",   ATOM10_PUBLISHED, NULL,            &published_end},
	{"updated",     ATOM10_UPDATED,   NULL,            &updated_end},
	{"author",      ATOM10_AUTHOR,    &author_start,   NULL},
	{"contributor", ATOM10_AUTHOR,    &author_start,   NULL},
	{"name",        ATOM10_NAME,      NULL,            &name_end},
	{"uri",         ATOM10_URI,       NULL,            &uri_end},
	{"email",       ATOM10_EMAIL,     NULL,            &email_end},
	{"category",    ATOM10_NONE,      &category_start, NULL},
	{"subtitle",    ATOM10_SUBTITLE,  NULL,            &subtitle_end},
	{"generator",   ATOM10_GENERATOR, NULL,            &generator_end},
	{NULL,          ATOM10_NONE,      NULL,            NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_ATOM10
