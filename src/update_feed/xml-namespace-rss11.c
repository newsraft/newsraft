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

static inline void
item_start(struct parser_data *data)
{
	data->rss11_pos |= RSS11_ITEM;
}

static inline void
item_end(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_ITEM) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_ITEM;
	insert_item(data->feed_url, data->item);
	empty_item_bucket(data->item);
}

static inline void
title_start(struct parser_data *data)
{
	data->rss11_pos |= RSS11_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_TITLE) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_TITLE;
	if ((data->rss11_pos & RSS11_ITEM) != 0) {
		if (cpyss(data->item->title, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->title, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
link_start(struct parser_data *data)
{
	data->rss11_pos |= RSS11_LINK;
}

static inline void
link_end(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_LINK) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_LINK;
	if ((data->rss11_pos & RSS11_ITEM) != 0) {
		if (cpyss(data->item->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->link, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
description_start(struct parser_data *data)
{
	data->rss11_pos |= RSS11_DESCRIPTION;
}

static inline void
description_end(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_DESCRIPTION) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_DESCRIPTION;
	if ((data->rss11_pos & RSS11_ITEM) != 0) {
		if (cpyss(data->item->content, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->summary, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
image_start(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_ITEM) != 0) {
		// In RSS 0.9, RSS 1.0 and RSS 1.1 items can not have image elements.
		return;
	}
	data->rss11_pos |= RSS11_IMAGE;
}

static inline void
image_end(struct parser_data *data)
{
	if ((data->rss11_pos & RSS11_IMAGE) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_IMAGE;
}

static inline void
url_start(struct parser_data *data)
{
	data->rss11_pos |= RSS11_URL;
}

static inline void
url_end(struct parser_data *data)
{
	(void)data;
	if ((data->rss11_pos & RSS11_URL) == 0) {
		return;
	}
	data->rss11_pos &= ~RSS11_URL;
	if ((data->rss11_pos & RSS11_IMAGE) != 0) {
		// TODO: update image url of the feed
	}
}

void
parse_rss11_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "item") == 0)        { item_start(data);        }
	else if (strcmp(name, "title") == 0)       { title_start(data);       }
	else if (strcmp(name, "link") == 0)        { link_start(data);        }
	else if (strcmp(name, "description") == 0) { description_start(data); }
	else if (strcmp(name, "image") == 0)       { image_start(data);       }
	else if (strcmp(name, "url") == 0)         { url_start(data);         }
}

void
parse_rss11_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "item") == 0)        { item_end(data);        }
	else if (strcmp(name, "title") == 0)       { title_end(data);       }
	else if (strcmp(name, "link") == 0)        { link_end(data);        }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "image") == 0)       { image_end(data);       }
	else if (strcmp(name, "url") == 0)         { url_end(data);         }
}
#endif // FEEDEATER_FORMAT_SUPPORT_RSS11
