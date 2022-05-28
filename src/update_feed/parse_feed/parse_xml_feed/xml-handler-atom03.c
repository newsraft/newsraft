#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20210303084351/https://pythonhosted.org/feedparser/annotated-atom03.html
// https://web.archive.org/web/20210417183310/http://rakaz.nl/2005/07/moving-from-atom-03-to-10.html
// https://web.archive.org/web/20220502094905/https://support.google.com/merchants/answer/160598?hl=en

// Note to the future.
// Atom 0.3 does not have category element.

static void
entry_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	prepend_item(&data->feed.item);
}

static void
id_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return; // Ignore feed id.
	}
	if (crtss_or_cpyss(&data->feed.item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
title_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (copy_type_of_text_construct(&data->feed.item->title.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (copy_type_of_text_construct(&data->feed.title.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
title_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
link_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	// Atom 0.3 link element doesn't have length and hreflang attributes.
	const char *href = get_value_of_attribute_key(attrs, "href");
	if (href == NULL) {
		return;
	}
	const size_t href_len = strlen(href);
	if (href_len == 0) {
		return;
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		// Ignore links to feed itself.
		return;
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
			if (crtas_or_cpyas(&data->feed.item->url, href, href_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			if (crtas_or_cpyas(&data->feed.url, href, href_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (prepend_link(&data->feed.item->attachment) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtas_or_cpyas(&data->feed.item->attachment->url, href, href_len) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		const char *type = get_value_of_attribute_key(attrs, "type");
		if (type != NULL) {
			if (crtas_or_cpyas(&data->feed.item->attachment->type, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static void
summary_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (copy_type_of_text_construct(&data->feed.item->summary.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
summary_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (copy_type_of_text_construct(&data->feed.item->content.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
content_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
issued_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		// Atom 0.3 feed can have issued date but who needs it?
		return;
	}
	data->feed.item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
}

static void
modified_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		data->feed.item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed.update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static void
author_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (prepend_person(&data->feed.item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (prepend_person(&data->feed.author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
name_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
url_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
email_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (crtss_or_cpyss(&data->feed.item->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (crtss_or_cpyss(&data->feed.author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
tagline_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	if (copy_type_of_text_construct(&data->feed.summary.type, attrs) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
tagline_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed.summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
generator_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	size_t attr_len;
	const char *attr = get_value_of_attribute_key(attrs, "version");
	if (attr != NULL) {
		attr_len = strlen(attr);
		if (attr_len != 0) {
			if (crtas_or_cpyas(&data->feed.generator.version, attr, attr_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
	attr = get_value_of_attribute_key(attrs, "url");
	if (attr != NULL) {
		attr_len = strlen(attr);
		if (attr_len != 0) {
			if (crtas_or_cpyas(&data->feed.generator.url, attr, attr_len) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static void
generator_end(struct stream_callback_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	if (crtss_or_cpyss(&data->feed.generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_atom03_handlers[] = {
	{"entry",       ATOM03_ENTRY,     &entry_start,     NULL},
	{"id",          ATOM03_ID,        NULL,             &id_end},
	{"title",       ATOM03_TITLE,     &title_start,     &title_end},
	{"link",        ATOM03_NONE,      &link_start,      NULL},
	{"summary",     ATOM03_SUMMARY,   &summary_start,   &summary_end},
	{"content",     ATOM03_CONTENT,   &content_start,   &content_end},
	{"issued",      ATOM03_ISSUED,    NULL,             &issued_end},
	{"modified",    ATOM03_MODIFIED,  NULL,             &modified_end},
	{"author",      ATOM03_AUTHOR,    &author_start,    NULL},
	{"contributor", ATOM03_AUTHOR,    &author_start,    NULL},
	{"name",        ATOM03_NAME,      NULL,             &name_end},
	{"url",         ATOM03_URL,       NULL,             &url_end},
	{"email",       ATOM03_EMAIL,     NULL,             &email_end},
	{"tagline",     ATOM03_TAGLINE,   &tagline_start,   &tagline_end},
	{"generator",   ATOM03_GENERATOR, &generator_start, &generator_end},
	{NULL,          ATOM03_NONE,      NULL,             NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_ATOM03
