#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// Despite this file named "xml-namespace-rss11.c", it is actually
// considered to parse RSS 0.9, RSS 1.0 and RSS 1.1 feeds.
//
// Some useful notes:
//
// All of these formats do support image enclosures with the tag <image>
// but the flaw is that you can't include image elements in items.
// They are only for channel overall...
//
// https://web.archive.org/web/20211011074123/https://www.rssboard.org/rss-0-9-0
// https://web.archive.org/web/20211106023928/https://web.resource.org/rss/1.0/spec
// https://web.archive.org/web/20210411040907/http://inamidst.com/rss1.1/

enum rss11_position {
	RSS11_NONE = 0,
	RSS11_ITEM = 1,
	RSS11_TITLE = 2,
	RSS11_LINK = 4,
	RSS11_DESCRIPTION = 8,
	RSS11_IMAGE = 16,
	RSS11_URL = 32,
};

int8_t rss11_pos;

static inline void
item_start(void)
{
	rss11_pos |= RSS11_ITEM;
}

static inline void
item_end(struct parser_data *data)
{
	if ((rss11_pos & RSS11_ITEM) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_ITEM;
	try_item_bucket(data->bucket, data->feed_url);
	empty_item_bucket(data->bucket);
}

static inline void
title_start(void)
{
	rss11_pos |= RSS11_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((rss11_pos & RSS11_TITLE) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_TITLE;
	if ((rss11_pos & RSS11_ITEM) != 0) {
		if (cpyas(data->bucket->title, data->value, data->value_len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	}
}

static inline void
link_start(void)
{
	rss11_pos |= RSS11_LINK;
}

static inline void
link_end(struct parser_data *data)
{
	if ((rss11_pos & RSS11_LINK) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_LINK;
	if ((rss11_pos & RSS11_ITEM) != 0) {
		if (cpyas(data->bucket->url, data->value, data->value_len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		db_update_feed_text(data->feed_url, "resource", data->value, data->value_len);
	}
}

static inline void
description_start(void)
{
	rss11_pos |= RSS11_DESCRIPTION;
}

static inline void
description_end(struct parser_data *data)
{
	if ((rss11_pos & RSS11_DESCRIPTION) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_DESCRIPTION;
	if ((rss11_pos & RSS11_ITEM) != 0) {
		if (cpyas(data->bucket->content, data->value, data->value_len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	}
}

static inline void
image_start(void)
{
	if ((rss11_pos & RSS11_ITEM) != 0) {
		// In RSS 0.9, RSS 1.0 and RSS 1.1 items can not have image elements.
		return;
	}
	rss11_pos |= RSS11_IMAGE;
}

static inline void
image_end(void)
{
	if ((rss11_pos & RSS11_IMAGE) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_IMAGE;
}

static inline void
url_start(void)
{
	rss11_pos |= RSS11_URL;
}

static inline void
url_end(struct parser_data *data)
{
	(void)data;
	if ((rss11_pos & RSS11_URL) == 0) {
		return;
	}
	rss11_pos &= ~RSS11_URL;
	if ((rss11_pos & RSS11_IMAGE) != 0) {
		// TODO: update image url of the feed
	}
}

void XMLCALL
parse_rss11_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)data;
	(void)atts;
	     if (strcmp(name, "item") == 0)        { item_start();        }
	else if (strcmp(name, "title") == 0)       { title_start();       }
	else if (strcmp(name, "link") == 0)        { link_start();        }
	else if (strcmp(name, "description") == 0) { description_start(); }
	else if (strcmp(name, "image") == 0)       { image_start();       }
	else if (strcmp(name, "url") == 0)         { url_start();         }
}

void XMLCALL
parse_rss11_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "item") == 0)        { item_end(data);        }
	else if (strcmp(name, "title") == 0)       { title_end(data);       }
	else if (strcmp(name, "link") == 0)        { link_end(data);        }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "image") == 0)       { image_end();           }
	else if (strcmp(name, "url") == 0)         { url_end(data);         }
}
#endif // FEEDEATER_FORMAT_SUPPORT_RSS11
