#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
#include <stdio.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static inline void
entry_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_ENTRY;
}

static inline void
entry_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_ENTRY;
	insert_item(data->feed_url, &data->item);
	empty_item_bucket(&data->item);
}

static inline void
title_start(struct parser_data *data, const XML_Char **atts)
{
	data->atom10_pos |= ATOM10_TITLE;
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if (cpyas(data->item.title_type, type, strlen(type)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
title_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_TITLE) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_TITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyss(data->item.title, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed.title, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
link_start(struct parser_data *data, const XML_Char **atts)
{
	const char *href = NULL, *type = NULL, *length = NULL;
	bool this_is_enclosure = false;
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "href") == 0) {
			href = atts[i + 1];
		} else if (strcmp(atts[i], "type") == 0) {
			type = atts[i + 1];
		} else if (strcmp(atts[i], "length") == 0) {
			length = atts[i + 1];
		} else if (strcmp(atts[i], "rel") == 0) {
			if (strcmp(atts[i + 1], "alternate") == 0) {
				this_is_enclosure = false;
			} else if (strcmp(atts[i + 1], "self") == 0) {
				// If link has "rel" attribute with "self" value then this
				// link points to the feed itself, we don't need it.
				return;
			} else {
				this_is_enclosure = true;
			}
		}
	}
	if (this_is_enclosure == true) {
		if (expand_link_list_by_one_element(&data->item.enclosures) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (href != NULL) {
			if (add_url_to_last_link(&data->item.enclosures, href, strlen(href)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (type != NULL) {
			if (add_type_to_last_link(&data->item.enclosures, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (length != NULL) {
			// Do not check this call for errors, because its fail is not fatal. Everything that
			// can go wrong is failure on sscanf owing to invalid (non-integer) value of length.
			add_size_to_last_link(&data->item.enclosures, length);
		}
	} else {
		if (href != NULL) {
			if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
				if (cpyas(data->item.url, href, strlen(href)) == false) {
					data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
					return;
				}
			} else {
				if (cpyas(data->feed.link, href, strlen(href)) == false) {
					data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
					return;
				}
			}
		}
		// TODO: make item.url of struct link * type and set type and length
	}
}

static inline void
summary_start(struct parser_data *data, const XML_Char **atts)
{
	data->atom10_pos |= ATOM10_SUMMARY;
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom10_pos & ATOM10_ENTRY) != 0)) {
		if (cpyas(data->item.summary_type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
summary_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_SUMMARY) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUMMARY;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyss(data->item.summary, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
content_start(struct parser_data *data, const XML_Char **atts)
{
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom10_pos & ATOM10_ENTRY) != 0)) {
		if (cpyas(data->item.content_type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	data->atom10_pos |= ATOM10_CONTENT;
}

static inline void
content_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_CONTENT) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_CONTENT;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->item.content, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
id_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_ID;
}

static inline void
id_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_ID) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_ID;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// In Atom 1.0 feed can have unique id, but who needs it?
		return;
	}
	if (cpyss(data->item.guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
published_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_PUBLISHED;
}

static inline void
published_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_PUBLISHED) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_PUBLISHED;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		data->item.pubdate = parse_date_rfc3339(data->value);
	}
}

static inline void
updated_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_UPDATED;
}

static inline void
updated_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_UPDATED) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_UPDATED;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		data->item.upddate = parse_date_rfc3339(data->value);
	}
}

static inline void
author_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_AUTHOR;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if (expand_person_list_by_one_element(&data->item.authors) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
author_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_AUTHOR;
}

static inline void
name_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_NAME;
}

static inline void
name_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_NAME) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_NAME;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if (add_name_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
uri_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_URI;
}

static inline void
uri_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_URI) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_URI;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if (add_link_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
email_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_EMAIL;
}

static inline void
email_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_EMAIL) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_EMAIL;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if (add_email_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
category_start(struct parser_data *data, const XML_Char **atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global categories, but who needs it?
		return;
	}
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "term") == 0) {
			if (atts[i + 1] != NULL) {
				if (add_category_to_item_bucket(&data->item, atts[i + 1], strlen(atts[i + 1])) == false) {
					data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
					return;
				}
			}
			break;
		}
	}
}

static inline void
subtitle_start(struct parser_data *data, const XML_Char **atts)
{
	data->atom10_pos |= ATOM10_SUBTITLE;
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom10_pos & ATOM10_ENTRY) == 0)) {
		if (cpyas(data->feed.summary_type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
subtitle_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_SUBTITLE) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUBTITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		if (cpyss(data->feed.summary, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

void
parse_atom10_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	     if (strcmp(name, "entry")       == 0) { entry_start(data);          }
	else if (strcmp(name, "id")          == 0) { id_start(data);             }
	else if (strcmp(name, "title")       == 0) { title_start(data, atts);    }
	else if (strcmp(name, "link")        == 0) { link_start(data, atts);     }
	else if (strcmp(name, "summary")     == 0) { summary_start(data, atts);  }
	else if (strcmp(name, "content")     == 0) { content_start(data, atts);  }
	else if (strcmp(name, "published")   == 0) { published_start(data);      }
	else if (strcmp(name, "updated")     == 0) { updated_start(data);        }
	else if (strcmp(name, "author")      == 0) { author_start(data);         }
	else if (strcmp(name, "contributor") == 0) { author_start(data);         }
	else if (strcmp(name, "name")        == 0) { name_start(data);           }
	else if (strcmp(name, "uri")         == 0) { uri_start(data);            }
	else if (strcmp(name, "email")       == 0) { email_start(data);          }
	else if (strcmp(name, "category")    == 0) { category_start(data, atts); }
	else if (strcmp(name, "subtitle")    == 0) { subtitle_start(data, atts); }
}

void
parse_atom10_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "entry")       == 0) { entry_end(data);     }
	else if (strcmp(name, "id")          == 0) { id_end(data);        }
	else if (strcmp(name, "title")       == 0) { title_end(data);     }
	else if (strcmp(name, "summary")     == 0) { summary_end(data);   }
	else if (strcmp(name, "content")     == 0) { content_end(data);   }
	else if (strcmp(name, "published")   == 0) { published_end(data); }
	else if (strcmp(name, "updated")     == 0) { updated_end(data);   }
	else if (strcmp(name, "author")      == 0) { author_end(data);    }
	else if (strcmp(name, "contributor") == 0) { author_end(data);    }
	else if (strcmp(name, "name")        == 0) { name_end(data);      }
	else if (strcmp(name, "uri")         == 0) { uri_end(data);       }
	else if (strcmp(name, "email")       == 0) { email_end(data);     }
	else if (strcmp(name, "subtitle")    == 0) { subtitle_end(data);  }
	// In Atom 1.0 link tag is a self-closing tag.
	//else if (strcmp(name, "link")     == 0) {                       }
	// In Atom 1.0 category tag is a self-closing tag.
	//else if (strcmp(name, "category") == 0) {                       }
}
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM10
