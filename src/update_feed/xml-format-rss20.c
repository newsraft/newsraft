#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

enum rss20_position {
	RSS20_NONE = 0,
	RSS20_ITEM = 1,
	RSS20_TITLE = 2,
	RSS20_DESCRIPTION = 4,
	RSS20_LINK = 8,
	RSS20_PUBDATE = 16,
	RSS20_GUID = 32,
	RSS20_AUTHOR = 64,
	RSS20_ENCLOSURE = 128,
	RSS20_SOURCE = 256,
	RSS20_CATEGORY = 512,
	RSS20_COMMENTS = 1024,
	RSS20_CHANNEL = 2048,
};

int64_t rss20_pos = RSS20_NONE;

static inline void
item_start(void)
{
	rss20_pos |= RSS20_ITEM;
}

static inline void
item_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_ITEM;
	try_item_bucket(data->bucket, data->feed_url);
	drop_item_bucket(data->bucket);
}

static inline void
title_start(void)
{
	rss20_pos |= RSS20_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_TITLE;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		cpyas(data->bucket->title, data->value, data->value_len);
	} else {
		db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
	}
}

static inline void
link_start(void)
{
	rss20_pos |= RSS20_LINK;
}

static inline void
link_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_LINK;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		cpyas(data->bucket->url, data->value, data->value_len);
	} else {
		db_update_feed_text(data->feed_url, "resource", data->value, data->value_len);
	}
}

static inline void
description_start(void)
{
	rss20_pos |= RSS20_DESCRIPTION;
}

static inline void
description_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_DESCRIPTION;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		cpyas(data->bucket->content, data->value, data->value_len);
	} else {
		db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	}
}

static inline void
pubDate_start(void)
{
	rss20_pos |= RSS20_PUBDATE;
}

static inline void
pubDate_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_PUBDATE;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		data->bucket->pubdate = parse_date_rfc822(data->value, data->value_len);
	} else {
		// RSS 2.0 says that channel can have pubDate element, but who needs it?
	}
}

static inline void
guid_start(void)
{
	rss20_pos |= RSS20_GUID;
}

static inline void
guid_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_GUID;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		cpyas(data->bucket->guid, data->value, data->value_len);
	}
}

static inline void
author_start(struct parser_data *data)
{
	rss20_pos |= RSS20_AUTHOR;
	if ((rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	++(data->bucket->authors_count);
	data->bucket->authors = realloc(data->bucket->authors, sizeof(struct author) * data->bucket->authors_count);
	if (data->bucket->authors != NULL) {
		data->bucket->authors[data->bucket->authors_count - 1].name = NULL;
		data->bucket->authors[data->bucket->authors_count - 1].link = NULL;
		data->bucket->authors[data->bucket->authors_count - 1].email = NULL;
	}
}

static inline void
author_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_AUTHOR;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		if ((data->bucket->authors != NULL) &&
		    (data->bucket->authors_count != 0) &&
		    (data->bucket->authors[data->bucket->authors_count - 1].email == NULL))
		{
			data->bucket->authors[data->bucket->authors_count - 1].email = create_string(data->value, data->value_len);
		}
	}
}

static inline void
enclosure_start(struct parser_data *data, const XML_Char **atts)
{
	rss20_pos |= RSS20_ENCLOSURE;
	if ((rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	const char *url = NULL, *type = NULL;
	int length = 0;
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "url") == 0) {
			url = atts[i + 1];
		} else if (strcmp(atts[i], "type") == 0) {
			type = atts[i + 1];
		} else if (strcmp(atts[i], "length") == 0) {
			sscanf(atts[i + 1], "%d", &length);
		}
	}
	if (add_enclosure_to_item_bucket(data->bucket, url, type, length, 0) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
}

static inline void
enclosure_end(void)
{
	rss20_pos &= ~RSS20_ENCLOSURE;
}

static inline void
category_start(void)
{
	rss20_pos |= RSS20_CATEGORY;
}

static inline void
category_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_CATEGORY;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		add_category_to_item_bucket(data->bucket, data->value, data->value_len);
	} else {
		// RSS 2.0 says that channel can have category elements, but who needs them?
	}
}

static inline void
comments_start(void)
{
	rss20_pos |= RSS20_COMMENTS;
}

static inline void
comments_end(struct parser_data *data)
{
	rss20_pos &= ~RSS20_COMMENTS;
	if ((rss20_pos & RSS20_ITEM) != 0) {
		cpyas(data->bucket->comments, data->value, data->value_len);
	}
}

static inline void
channel_start(void)
{
	rss20_pos |= RSS20_CHANNEL;
}

static inline void
channel_end(void)
{
	rss20_pos &= ~RSS20_CHANNEL;
}

void XMLCALL
parse_rss20_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "item")        == 0) { item_start();                }
	else if (strcmp(name, "title")       == 0) { title_start();               }
	else if (strcmp(name, "link")        == 0) { link_start();                }
	else if (strcmp(name, "description") == 0) { description_start();         }
	else if (strcmp(name, "pubDate")     == 0) { pubDate_start();             }
	else if (strcmp(name, "guid")        == 0) { guid_start();                }
	else if (strcmp(name, "author")      == 0) { author_start(data);          }
	else if (strcmp(name, "enclosure")   == 0) { enclosure_start(data, atts); }
	else if (strcmp(name, "category")    == 0) { category_start();            }
	else if (strcmp(name, "comments")    == 0) { comments_start();            }
	else if (strcmp(name, "channel")     == 0) { channel_start();             }
}

void XMLCALL
parse_rss20_element_end(struct parser_data *data, const XML_Char *name)
{
	if ((rss20_pos & RSS20_CHANNEL) == 0) {
		return;
	}
	     if (strcmp(name, "item")        == 0) { item_end(data);        }
	else if (strcmp(name, "title")       == 0) { title_end(data);       }
	else if (strcmp(name, "link")        == 0) { link_end(data);        }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "pubDate")     == 0) { pubDate_end(data);     }
	else if (strcmp(name, "guid")        == 0) { guid_end(data);        }
	else if (strcmp(name, "author")      == 0) { author_end(data);      }
	else if (strcmp(name, "enclosure")   == 0) { enclosure_end();       }
	else if (strcmp(name, "category")    == 0) { category_end(data);    }
	else if (strcmp(name, "comments")    == 0) { comments_end(data);    }
	else if (strcmp(name, "channel")     == 0) { channel_end();         }
}
