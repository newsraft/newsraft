#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

static inline void
item_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_ITEM;
	prepend_item(&data->feed->item);
}

static inline void
item_end(struct xml_data *data)
{
	data->rss20_pos &= ~RSS20_ITEM;
}

static inline void
title_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_TITLE;
}

static inline void
title_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_TITLE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_TITLE;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
link_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_LINK;
}

static inline void
link_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_LINK) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_LINK;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
description_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_DESCRIPTION;
}

static inline void
description_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_DESCRIPTION) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_DESCRIPTION;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
pubDate_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_PUBDATE;
}

static inline void
pubDate_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_PUBDATE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_PUBDATE;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		data->feed->item->pubdate = parse_date_rfc822(data->value);
	} else {
		// Some RSS 2.0 feeds use lastBuildDate and some
		// use pubDate for showing last update time of channel.
		// But lastBuildDate is more commonly used, so don't
		// bother with pubDate value if lastBuildDate was already
		// set.
		if (data->feed->update_time == 0) {
			data->feed->update_time = parse_date_rfc822(data->value);
		}
	}
}

static inline void
guid_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_GUID;
}

static inline void
guid_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_GUID) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_GUID;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
author_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_AUTHOR;
}

static inline void
author_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_AUTHOR) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_AUTHOR;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (prepend_person(&data->feed->item->author) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyss(data->feed->item->author->email, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
enclosure_start(struct xml_data *data, const XML_Char **atts)
{
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (prepend_link(&data->feed->item->attachment) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "url") == 0) {
			if (cpyas(data->feed->item->attachment->url, atts[i + 1], strlen(atts[i + 1])) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else if (strcmp(atts[i], "type") == 0) {
			if (cpyas(data->feed->item->attachment->type, atts[i + 1], strlen(atts[i + 1])) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else if (strcmp(atts[i], "length") == 0) {
			data->feed->item->attachment->size = convert_string_to_size_t_or_zero(atts[i + 1]);
		}
	}
}

static inline void
category_start(struct xml_data *data, const XML_Char **atts)
{
	data->rss20_pos |= RSS20_CATEGORY;
	const char *domain = get_value_of_attribute_key(atts, "domain");
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
category_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_CATEGORY) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_CATEGORY;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
lastBuildDate_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_LASTBUILDDATE;
}

static inline void
lastBuildDate_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_LASTBUILDDATE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_LASTBUILDDATE;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		// In RSS 2.0 lastBuildDate element is only for channel,
		// for items they use pubDate.
		data->feed->update_time = parse_date_rfc822(data->value);
	}
}

static inline void
comments_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_COMMENTS;
}

static inline void
comments_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_COMMENTS) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_COMMENTS;
	if ((data->rss20_pos & RSS20_ITEM) == 0) {
		return;
	}
	if (cpyss(data->feed->item->comments_url, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
language_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_LANGUAGE;
}

static inline void
language_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_LANGUAGE) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_LANGUAGE;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		return;
	}
	if (cpyss(data->feed->language, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
webMaster_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_WEBMASTER;
}

static inline void
webMaster_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_WEBMASTER) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_WEBMASTER;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
managingEditor_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_MANAGINGEDITOR;
}

static inline void
managingEditor_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_MANAGINGEDITOR) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_MANAGINGEDITOR;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
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

static inline void
generator_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_GENERATOR;
}

static inline void
generator_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_GENERATOR) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_GENERATOR;
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		return;
	}
	if (cpyss(data->feed->generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
channel_start(struct xml_data *data)
{
	data->rss20_pos |= RSS20_CHANNEL;
}

static inline void
channel_end(struct xml_data *data)
{
	if ((data->rss20_pos & RSS20_CHANNEL) == 0) {
		return;
	}
	data->rss20_pos &= ~RSS20_CHANNEL;
}

void
parse_rss20_element_start(struct xml_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "item")           == 0) { item_start(data);            }
	else if (strcmp(name, "title")          == 0) { title_start(data);           }
	else if (strcmp(name, "link")           == 0) { link_start(data);            }
	else if (strcmp(name, "description")    == 0) { description_start(data);     }
	else if (strcmp(name, "pubDate")        == 0) { pubDate_start(data);         }
	else if (strcmp(name, "guid")           == 0) { guid_start(data);            }
	else if (strcmp(name, "author")         == 0) { author_start(data);          }
	else if (strcmp(name, "enclosure")      == 0) { enclosure_start(data, atts); }
	else if (strcmp(name, "category")       == 0) { category_start(data, atts);  }
	else if (strcmp(name, "lastBuildDate")  == 0) { lastBuildDate_start(data);   }
	else if (strcmp(name, "comments")       == 0) { comments_start(data);        }
	else if (strcmp(name, "language")       == 0) { language_start(data);        }
	else if (strcmp(name, "generator")      == 0) { generator_start(data);       }
	else if (strcmp(name, "webMaster")      == 0) { webMaster_start(data);       }
	else if (strcmp(name, "managingEditor") == 0) { managingEditor_start(data);  }
	else if (strcmp(name, "channel")        == 0) { channel_start(data);         }
}

void
parse_rss20_element_end(struct xml_data *data, const XML_Char *name)
{
	if ((data->rss20_pos & RSS20_CHANNEL) == 0) {
		return;
	}
	     if (strcmp(name, "item")           == 0) { item_end(data);           }
	else if (strcmp(name, "title")          == 0) { title_end(data);          }
	else if (strcmp(name, "link")           == 0) { link_end(data);           }
	else if (strcmp(name, "description")    == 0) { description_end(data);    }
	else if (strcmp(name, "pubDate")        == 0) { pubDate_end(data);        }
	else if (strcmp(name, "guid")           == 0) { guid_end(data);           }
	else if (strcmp(name, "author")         == 0) { author_end(data);         }
	else if (strcmp(name, "category")       == 0) { category_end(data);       }
	else if (strcmp(name, "lastBuildDate")  == 0) { lastBuildDate_end(data);  }
	else if (strcmp(name, "comments")       == 0) { comments_end(data);       }
	else if (strcmp(name, "language")       == 0) { language_end(data);       }
	else if (strcmp(name, "generator")      == 0) { generator_end(data);      }
	else if (strcmp(name, "webMaster")      == 0) { webMaster_end(data);      }
	else if (strcmp(name, "managingEditor") == 0) { managingEditor_end(data); }
	else if (strcmp(name, "channel")        == 0) { channel_end(data);        }
	// In RSS 2.0 enclosure tag is a self-closing tag.
	//else if (strcmp(name, "enclosure") == 0) {                            }
}
#endif // FEEDEATER_FORMAT_SUPPORT_RSS20
