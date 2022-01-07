#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

static inline void
item_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_ITEM;
}

static inline void
item_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_ITEM;
	insert_item(data->feed_url, &data->item);
	empty_item_bucket(&data->item);
}

static inline void
title_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_TITLE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_TITLE;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		if (cpyss(data->item.title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed.title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
link_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_LINK;
}

static inline void
link_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_LINK) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_LINK;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		if (cpyss(data->item.url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed.link, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
description_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_DESCRIPTION;
}

static inline void
description_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_DESCRIPTION) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_DESCRIPTION;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		if (cpyss(data->item.content.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed.summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
pubDate_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_PUBDATE;
}

static inline void
pubDate_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_PUBDATE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_PUBDATE;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		// RSS 2.0 says that channel can have pubDate element, but who needs it?
		return;
	}
	data->item.pubdate = parse_date_rfc822(data->value);
}

static inline void
guid_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_GUID;
}

static inline void
guid_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_GUID) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_GUID;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->item.guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
author_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_AUTHOR;
}

static inline void
author_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_AUTHOR) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_AUTHOR;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (expand_person_list_by_one_element(&data->item.authors) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (add_email_to_last_person(&data->item.authors, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
enclosure_start(struct parser_data *data, const XML_Char **atts)
{
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (expand_link_list_by_one_element(&data->item.enclosures) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "url") == 0) {
			if (add_url_to_last_link(&data->item.enclosures, atts[i + 1], strlen(atts[i + 1])) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else if (strcmp(atts[i], "type") == 0) {
			if (add_type_to_last_link(&data->item.enclosures, atts[i + 1], strlen(atts[i + 1])) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else if (strcmp(atts[i], "length") == 0) {
			// Do not check this call for errors, because its fail is not fatal. Everything that
			// can go wrong is failure on sscanf owing to invalid (non-integer) value of length.
			add_size_to_last_link(&data->item.enclosures, atts[i + 1], strlen(atts[i + 1]));
		}
	}
}

static inline void
category_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_CATEGORY;
}

static inline void
category_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_CATEGORY) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_CATEGORY;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		// RSS 2.0 says that channel can have category elements, but who needs them?
		return;
	}
	if (add_category_to_item_bucket(&data->item, data->value->ptr, data->value->len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
comments_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_COMMENTS;
}

static inline void
comments_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_COMMENTS) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_COMMENTS;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->item.comments_url, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
language_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_LANGUAGE;
}

static inline void
language_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_LANGUAGE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_LANGUAGE;
	if (cpyss(data->feed.language, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
channel_start(struct parser_data *data)
{
	data->rss20_pos |= RSS20_CHANNEL;
}

static inline void
channel_end(struct parser_data *data)
{
	if ((data->rss20_pos & RSS20_CHANNEL) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_CHANNEL;
}

void
parse_rss20_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "item")        == 0) { item_start(data);            }
	else if (strcmp(name, "title")       == 0) { title_start(data);           }
	else if (strcmp(name, "link")        == 0) { link_start(data);            }
	else if (strcmp(name, "description") == 0) { description_start(data);     }
	else if (strcmp(name, "pubDate")     == 0) { pubDate_start(data);         }
	else if (strcmp(name, "guid")        == 0) { guid_start(data);            }
	else if (strcmp(name, "author")      == 0) { author_start(data);          }
	else if (strcmp(name, "enclosure")   == 0) { enclosure_start(data, atts); }
	else if (strcmp(name, "category")    == 0) { category_start(data);        }
	else if (strcmp(name, "comments")    == 0) { comments_start(data);        }
	else if (strcmp(name, "language")    == 0) { language_start(data);        }
	else if (strcmp(name, "channel")     == 0) { channel_start(data);         }
}

void
parse_rss20_element_end(struct parser_data *data, const XML_Char *name)
{
	if ((data->rss20_pos & RSS20_CHANNEL) == 0) {
		return;
	}
	     if (strcmp(name, "item")        == 0) { item_end(data);        }
	else if (strcmp(name, "title")       == 0) { title_end(data);       }
	else if (strcmp(name, "link")        == 0) { link_end(data);        }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "pubDate")     == 0) { pubDate_end(data);     }
	else if (strcmp(name, "guid")        == 0) { guid_end(data);        }
	else if (strcmp(name, "author")      == 0) { author_end(data);      }
	else if (strcmp(name, "category")    == 0) { category_end(data);    }
	else if (strcmp(name, "comments")    == 0) { comments_end(data);    }
	else if (strcmp(name, "language")    == 0) { language_end(data);    }
	else if (strcmp(name, "channel")     == 0) { channel_end(data);     }
	// In RSS 2.0 enclosure tag is a self-closing tag.
	//else if (strcmp(name, "enclosure") == 0) {                        }
}
#endif // FEEDEATER_FORMAT_SUPPORT_RSS20
