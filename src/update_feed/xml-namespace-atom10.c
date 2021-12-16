#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
#include <stdio.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

enum atom10_position {
	ATOM10_NONE = 0,
	ATOM10_ENTRY = 1,
	ATOM10_ID = 2,
	ATOM10_TITLE = 4,
	ATOM10_SUMMARY = 8,
	ATOM10_CONTENT = 16,
	ATOM10_PUBLISHED = 32,
	ATOM10_UPDATED = 64,
	ATOM10_AUTHOR = 128,
	ATOM10_NAME = 256,
	ATOM10_URI = 512,
	ATOM10_EMAIL = 1024,
};

int16_t atom10_pos;

static inline void
entry_start(void)
{
	atom10_pos |= ATOM10_ENTRY;
}

static inline void
entry_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_ENTRY;
	try_item_bucket(data->bucket, data->feed_url);
	empty_item_bucket(data->bucket);
}

static inline void
title_start(void)
{
	atom10_pos |= ATOM10_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_TITLE) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_TITLE;
	if ((atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyas(data->bucket->title, data->value, data->value_len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		db_update_feed_text(data->feed_url, "name", data->value, data->value_len);
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
		if (expand_enclosures_of_item_bucket_by_one_element(data->bucket) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (href != NULL) {
			if (add_url_to_last_enclosure_of_item_bucket(data->bucket, href, strlen(href)) != 0) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (type != NULL) {
			if (add_type_to_last_enclosure_of_item_bucket(data->bucket, type, strlen(type)) != 0) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (length != NULL) {
			// Do not check this call for errors, because its fail is not fatal. Everything that
			// can go wrong is failure on sscanf owing to invalid (non-integer) value of length.
			add_size_to_last_enclosure_of_item_bucket(data->bucket, length);
		}
	} else {
		if (href != NULL) {
			if ((atom10_pos & ATOM10_ENTRY) != 0) {
				if (cpyas(data->bucket->url, href, strlen(href)) != 0) {
					data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
					return;
				}
			} else {
				db_update_feed_text(data->feed_url, "resource", href, strlen(href));
			}
		}
		// TODO: make bucket->url of struct link * type and set type and length
	}
}

static inline void
summary_start(struct parser_data *data, const XML_Char **atts)
{
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((atom10_pos & ATOM10_ENTRY) != 0)) {
		if (cpyas(data->bucket->summary_type, type_str, strlen(type_str)) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	atom10_pos |= ATOM10_SUMMARY;
}

static inline void
summary_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_SUMMARY) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_SUMMARY;
	if ((atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyas(data->bucket->summary, data->value, data->value_len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		db_update_feed_text(data->feed_url, "description", data->value, data->value_len);
	}
}

static inline void
content_start(struct parser_data *data, const XML_Char **atts)
{
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((atom10_pos & ATOM10_ENTRY) != 0)) {
		if (cpyas(data->bucket->content_type, type_str, strlen(type_str)) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	atom10_pos |= ATOM10_CONTENT;
}

static inline void
content_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_CONTENT) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_CONTENT;
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	if (cpyas(data->bucket->content, data->value, data->value_len) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
id_start(void)
{
	atom10_pos |= ATOM10_ID;
}

static inline void
id_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_ID) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_ID;
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// In Atom 1.0 feed can have unique id, but who needs it?
		return;
	}
	if (cpyas(data->bucket->guid, data->value, data->value_len) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
published_start(void)
{
	atom10_pos |= ATOM10_PUBLISHED;
}

static inline void
published_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_PUBLISHED) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_PUBLISHED;
	if ((atom10_pos & ATOM10_ENTRY) != 0) {
		data->bucket->pubdate = parse_date_rfc3339(data->value, data->value_len);
	}
}

static inline void
updated_start(void)
{
	atom10_pos |= ATOM10_UPDATED;
}

static inline void
updated_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_UPDATED) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_UPDATED;
	if ((atom10_pos & ATOM10_ENTRY) != 0) {
		data->bucket->upddate = parse_date_rfc3339(data->value, data->value_len);
	}
}

static inline void
author_start(struct parser_data *data)
{
	atom10_pos |= ATOM10_AUTHOR;
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if (expand_authors_of_item_bucket_by_one_element(data->bucket) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
author_end(void)
{
	if ((atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_AUTHOR;
}

static inline void
name_start(void)
{
	atom10_pos |= ATOM10_NAME;
}

static inline void
name_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_NAME) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_NAME;
	if ((atom10_pos & ATOM10_AUTHOR) == 0) {
		// So far name tag can be found only in author element.
		return;
	}
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if (add_name_to_last_author_of_item_bucket(data->bucket, data->value, data->value_len) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
uri_start(void)
{
	atom10_pos |= ATOM10_URI;
}

static inline void
uri_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_URI) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_URI;
	if ((atom10_pos & ATOM10_AUTHOR) == 0) {
		// So far uri tag can be found only in author element.
		return;
	}
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if (add_link_to_last_author_of_item_bucket(data->bucket, data->value, data->value_len) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
email_start(void)
{
	atom10_pos |= ATOM10_EMAIL;
}

static inline void
email_end(struct parser_data *data)
{
	if ((atom10_pos & ATOM10_EMAIL) == 0) {
		return;
	}
	atom10_pos &= ~ATOM10_EMAIL;
	if ((atom10_pos & ATOM10_AUTHOR) == 0) {
		// So far email tag can be found only in author element.
		return;
	}
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global author, but who needs it?
		return;
	}
	if (add_email_to_last_author_of_item_bucket(data->bucket, data->value, data->value_len) != 0) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
category_start(struct parser_data *data, const XML_Char **atts)
{
	if ((atom10_pos & ATOM10_ENTRY) == 0) {
		// Atom 1.0 says that feed can have global categories, but who needs it?
		return;
	}
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "term") == 0) {
			add_category_to_item_bucket(data->bucket, atts[i + 1], strlen(atts[i + 1]));
			break;
		}
	}
}

void XMLCALL
parse_atom10_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	     if (strcmp(name, "entry")       == 0) { entry_start();              }
	else if (strcmp(name, "id")          == 0) { id_start();                 }
	else if (strcmp(name, "title")       == 0) { title_start();              }
	else if (strcmp(name, "link")        == 0) { link_start(data, atts);     }
	else if (strcmp(name, "summary")     == 0) { summary_start(data, atts);  }
	else if (strcmp(name, "content")     == 0) { content_start(data, atts);  }
	else if (strcmp(name, "published")   == 0) { published_start();          }
	else if (strcmp(name, "updated")     == 0) { updated_start();            }
	else if (strcmp(name, "author")      == 0) { author_start(data);         }
	else if (strcmp(name, "contributor") == 0) { author_start(data);         }
	else if (strcmp(name, "name")        == 0) { name_start();               }
	else if (strcmp(name, "uri")         == 0) { uri_start();                }
	else if (strcmp(name, "email")       == 0) { email_start();              }
	else if (strcmp(name, "category")    == 0) { category_start(data, atts); }
}

void XMLCALL
parse_atom10_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "entry")       == 0) { entry_end(data);     }
	else if (strcmp(name, "id")          == 0) { id_end(data);        }
	else if (strcmp(name, "title")       == 0) { title_end(data);     }
	else if (strcmp(name, "summary")     == 0) { summary_end(data);   }
	else if (strcmp(name, "content")     == 0) { content_end(data);   }
	else if (strcmp(name, "published")   == 0) { published_end(data); }
	else if (strcmp(name, "updated")     == 0) { updated_end(data);   }
	else if (strcmp(name, "author")      == 0) { author_end();        }
	else if (strcmp(name, "contributor") == 0) { author_end();        }
	else if (strcmp(name, "name")        == 0) { name_end(data);      }
	else if (strcmp(name, "uri")         == 0) { uri_end(data);       }
	else if (strcmp(name, "email")       == 0) { email_end(data);     }
	// In Atom 1.0 link tag is a self-closing tag.
	//else if (strcmp(name, "link")     == 0) {                     }
	// In Atom 1.0 category tag is a self-closing tag.
	//else if (strcmp(name, "category") == 0) {                     }
}
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM10
