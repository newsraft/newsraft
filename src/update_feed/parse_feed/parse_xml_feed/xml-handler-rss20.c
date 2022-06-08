#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
#include <stdio.h>
#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

static int8_t
item_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (prepend_item(&data->feed.item) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
guid_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.title.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.title.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
link_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->url, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.url, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->content.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->content.type, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.summary.value, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.summary.type, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
pub_date_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		data->feed.item->pubdate = parse_date_rfc822(data->value);
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		// Some RSS 2.0 feeds use lastBuildDate and some
		// use pubDate for showing last update time of channel.
		// But lastBuildDate is more commonly used, so don't
		// bother with pubDate value if lastBuildDate was already
		// set.
		if (data->feed.update_date == 0) {
			data->feed.update_date = parse_date_rfc822(data->value);
		}
	}
	return PARSE_OKAY;
}

static int8_t
last_build_date_end(struct stream_callback_data *data)
{
	// In RSS 2.0 lastBuildDate element is only for channel,
	// for items they use pubDate.
	if (data->path[data->depth] == RSS20_CHANNEL) {
		data->feed.update_date = parse_date_rfc822(data->value);
	}
	return PARSE_OKAY;
}

static int8_t
author_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (prepend_person(&data->feed.item->author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.item->author->email = crtss(data->value);
		if (data->feed.item->author->email == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (prepend_person(&data->feed.author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.author->email = crtss(data->value);
		if (data->feed.author->email == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
enclosure_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != RSS20_ITEM) {
		return PARSE_OKAY;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY;
	}
	size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY;
	}
	if (prepend_link(&data->feed.item->attachment) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data->feed.item->attachment->url = crtas(attr, attr_len);
	if (data->feed.item->attachment->url == NULL) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	attr = get_value_of_attribute_key(attrs, "type");
	if (attr != NULL) {
		attr_len = strlen(attr);
		if (attr_len != 0) {
			data->feed.item->attachment->type = crtas(attr, attr_len);
			if (data->feed.item->attachment->type == NULL) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	attr = get_value_of_attribute_key(attrs, "length");
	if (attr != NULL) {
		data->feed.item->attachment->size = convert_string_to_size_t_or_zero(attr);
	}
	return PARSE_OKAY;
}

static int8_t
category_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	const char *domain = get_value_of_attribute_key(attrs, "domain");
	size_t domain_len;
	if (data->path[data->depth] == RSS20_ITEM) {
		if (prepend_category(&data->feed.item->category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (domain != NULL) {
			domain_len = strlen(domain);
			if (domain_len != 0) {
				data->feed.item->category->scheme = crtas(domain, domain_len);
				if (data->feed.item->category->scheme == NULL) {
					return PARSE_FAIL_NOT_ENOUGH_MEMORY;
				}
			}
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (prepend_category(&data->feed.category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (domain != NULL) {
			domain_len = strlen(domain);
			if (domain_len != 0) {
				data->feed.category->scheme = crtas(domain, domain_len);
				if (data->feed.category->scheme == NULL) {
					return PARSE_FAIL_NOT_ENOUGH_MEMORY;
				}
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
category_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->category->term, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.category->term, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
comments_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->comments_url, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
ttl_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		int64_t minutes;
		if (sscanf(data->value->ptr, "%" SCNd64, &minutes) != 1) {
			WARN("Couldn't convert value of <ttl> element to a number!");
			return PARSE_OKAY; // Continue parsing like nothing happened.
		}
		if (minutes < 0) {
			WARN("Value of <ttl> element is negative!");
			return PARSE_OKAY; // Continue parsing like nothing happened.
		}
		data->feed.time_to_live = minutes * 60;
	}
	return PARSE_OKAY;
}

static int8_t
language_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.language, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
generator_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.generator.name, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
web_master_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (prepend_person(&data->feed.webmaster) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.webmaster->email = crtss(data->value);
		if (data->feed.webmaster->email == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
managing_editor_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (prepend_person(&data->feed.editor) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.editor->email = crtss(data->value);
		if (data->feed.editor->email == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rss20_handlers[] = {
	// <channel> is a container for all this stuff but it is quite redundant.
	{"item",           RSS20_ITEM,           &item_start,      NULL},
	{"guid",           RSS20_GUID,           NULL,             &guid_end},
	{"title",          RSS20_TITLE,          NULL,             &title_end},
	{"link",           RSS20_LINK,           NULL,             &link_end},
	{"description",    RSS20_DESCRIPTION,    NULL,             &description_end},
	{"pubDate",        RSS20_PUBDATE,        NULL,             &pub_date_end},
	{"lastBuildDate",  RSS20_LASTBUILDDATE,  NULL,             &last_build_date_end},
	{"author",         RSS20_AUTHOR,         NULL,             &author_end},
	{"enclosure",      XML_UNKNOWN_POS,      &enclosure_start, NULL},
	{"category",       RSS20_CATEGORY,       &category_start,  &category_end},
	{"comments",       RSS20_COMMENTS,       NULL,             &comments_end},
	{"ttl",            RSS20_TTL,            NULL,             &ttl_end},
	{"language",       RSS20_LANGUAGE,       NULL,             &language_end},
	{"generator",      RSS20_GENERATOR,      NULL,             &generator_end},
	{"webMaster",      RSS20_WEBMASTER,      NULL,             &web_master_end},
	{"managingEditor", RSS20_MANAGINGEDITOR, NULL,             &managing_editor_end},
	{"channel",        RSS20_CHANNEL,        NULL,             NULL},
	{NULL,             XML_UNKNOWN_POS,      NULL,             NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_RSS20
