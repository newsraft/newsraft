#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

static void
item_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	prepend_item(&data->feed->item);
}

static void
guid_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
title_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
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
link_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		if (cpyss(data->feed->item->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
description_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		if (cpyss(data->feed->item->content.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
pubDate_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		data->feed->item->pubdate = parse_date_rfc822(data->value);
	} else {
		// Some RSS 2.0 feeds use lastBuildDate and some
		// use pubDate for showing last update time of channel.
		// But lastBuildDate is more commonly used, so don't
		// bother with pubDate value if lastBuildDate was already
		// set.
		if (data->feed->update_date == 0) {
			data->feed->update_date = parse_date_rfc822(data->value);
		}
	}
}

static void
lastBuildDate_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) == 0) {
		// In RSS 2.0 lastBuildDate element is only for channel,
		// for items they use pubDate.
		data->feed->update_date = parse_date_rfc822(data->value);
	}
}

static void
author_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		if (prepend_person(&data->feed->item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyss(data->feed->item->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (prepend_person(&data->feed->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyss(data->feed->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
enclosure_start(struct xml_data *data, const TidyAttr attrs)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) == 0) {
		return;
	}
	const char *url = get_value_of_attribute_key(attrs, "url");
	if (url == NULL) {
		return;
	}
	const size_t url_len = strlen(url);
	if (url_len == 0) {
		return;
	}
	if (prepend_link(&data->feed->item->attachment) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyas(data->feed->item->attachment->url, url, url_len) == false) {
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
	const char *length = get_value_of_attribute_key(attrs, "length");
	if (length != NULL) {
		data->feed->item->attachment->size = convert_string_to_size_t_or_zero(length);
	}
}

static void
category_start(struct xml_data *data, const TidyAttr attrs)
{
	const char *domain = get_value_of_attribute_key(attrs, "domain");
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		if (prepend_category(&data->feed->item->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (domain != NULL) {
			if (cpyas(data->feed->item->category->scheme, domain, strlen(domain)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (prepend_category(&data->feed->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (domain != NULL) {
			if (cpyas(data->feed->category->scheme, domain, strlen(domain)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static void
category_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		if (data->feed->item->category == NULL) {
			return;
		}
		if (cpyss(data->feed->item->category->term, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (data->feed->category == NULL) {
			return;
		}
		if (cpyss(data->feed->category->term, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
comments_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->feed->item->comments_url, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
language_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		return;
	}
	if (cpyss(data->feed->language, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
generator_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		return;
	}
	if (cpyss(data->feed->generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
webMaster_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		return;
	}
	if (prepend_person(&data->feed->webmaster) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyss(data->feed->webmaster->email, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
managingEditor_end(struct xml_data *data)
{
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		return;
	}
	if (prepend_person(&data->feed->editor) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyss(data->feed->editor->email, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_rss20_handlers[] = {
	// <channel> is a container for all this stuff but it is quite redundant.
	{"item",           RSS20_ITEM,           &item_start,      NULL},
	{"guid",           RSS20_GUID,           NULL,             &guid_end},
	{"title",          RSS20_TITLE,          NULL,             &title_end},
	{"link",           RSS20_LINK,           NULL,             &link_end},
	{"description",    RSS20_DESCRIPTION,    NULL,             &description_end},
	{"pubDate",        RSS20_PUBDATE,        NULL,             &pubDate_end},
	{"lastBuildDate",  RSS20_LASTBUILDDATE,  NULL,             &lastBuildDate_end},
	{"author",         RSS20_AUTHOR,         NULL,             &author_end},
	{"enclosure",      RSS20_NONE,           &enclosure_start, NULL},
	{"category",       RSS20_CATEGORY,       &category_start,  &category_end},
	{"comments",       RSS20_COMMENTS,       NULL,             &comments_end},
	{"language",       RSS20_LANGUAGE,       NULL,             &language_end},
	{"generator",      RSS20_GENERATOR,      NULL,             &generator_end},
	{"webMaster",      RSS20_WEBMASTER,      NULL,             &webMaster_end},
	{"managingEditor", RSS20_MANAGINGEDITOR, NULL,             &managingEditor_end},
	{NULL,             RSS20_NONE,           NULL,             NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_RSS20
