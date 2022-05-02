#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20210303084351/https://pythonhosted.org/feedparser/annotated-atom03.html
// https://web.archive.org/web/20210417183310/http://rakaz.nl/2005/07/moving-from-atom-03-to-10.html
// https://web.archive.org/web/20220502094905/https://support.google.com/merchants/answer/160598?hl=en

// Note to the future.
// Atom 0.3 does not have category element.

static void
copy_type_of_text_construct(struct xml_data *data, const TidyAttr atts, struct string *dest)
{
	// Atom 0.3 text construct types are fully compliant with MIME standard.
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if (cpyas(dest, type, strlen(type)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		return;
	}
	if (cpyas(dest, "text/plain", 10) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
entry_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	prepend_item(&data->feed->item);
}

static void
id_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return; // Ignore feed id.
	}
	if (cpyss(data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
title_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		copy_type_of_text_construct(data, attrs, data->feed->item->title.type);
	} else {
		copy_type_of_text_construct(data, attrs, data->feed->title.type);
	}
}

static void
title_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (cpyss(data->feed->item->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
link_start(struct xml_data *data, const TidyAttr attrs)
{
	// Atom 0.3 link element does not have length and hreflang attributes.
	const char *href = get_value_of_attribute_key(attrs, "href");
	if (href == NULL) {
		// In Atom 0.3 links href attribute MUST be set.
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
			if (cpyas(data->feed->item->url, href, strlen(href)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			if (cpyas(data->feed->url, href, strlen(href)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (prepend_link(&data->feed->item->attachment) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyas(data->feed->item->attachment->url, href, strlen(href)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		const char *type = get_value_of_attribute_key(attrs, "type");
		if (type != NULL) {
			if (cpyas(data->feed->item->attachment->type, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static void
summary_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	copy_type_of_text_construct(data, attrs, data->feed->item->summary.type);
}

static void
summary_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->feed->item->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
content_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	copy_type_of_text_construct(data, attrs, data->feed->item->content.type);
}

static void
content_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->feed->item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
issued_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) == 0) {
		// Atom 0.3 feed can have issued date but who needs it?
		return;
	}
	data->feed->item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
}

static void
modified_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		data->feed->item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed->update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static void
author_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
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
name_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
url_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
email_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_AUTHOR) == 0) {
		return;
	}
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
tagline_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	copy_type_of_text_construct(data, attrs, data->feed->summary.type);
}

static void
tagline_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
generator_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	const char *version = get_value_of_attribute_key(attrs, "version");
	if (version != NULL) {
		if (cpyas(data->feed->generator.version, version, strlen(version)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *url = get_value_of_attribute_key(attrs, "url");
	if (url != NULL) {
		if (cpyas(data->feed->generator.url, url, strlen(url)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
generator_end(struct xml_data *data)
{
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed->generator.name, data->value) == false) {
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
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM03
