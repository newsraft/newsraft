#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
#include <string.h>
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static inline void
copy_type_of_text_construct(struct parser_data *data, const XML_Char **atts, struct string *target)
{
	// Valid Atom 1.0 text construct types are "text", "html" and "xhtml".
	// If the "type" attribute is not provided, then we MUST assume
	// that data is of "text" type (or "text/plain" in terms of MIME standard).
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if (strcmp(type, "html") == 0) {
			if (cpyas(target, "text/html", 9) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
			return;
		}
	}
	if (cpyas(target, "text/plain", 10) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

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
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		copy_type_of_text_construct(data, atts, data->item.title.type);
	} else {
		copy_type_of_text_construct(data, atts, data->feed.title.type);
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
			add_size_to_last_link(&data->item.enclosures, length, strlen(length));
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
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	data->atom10_pos |= ATOM10_SUMMARY;
	copy_type_of_text_construct(data, atts, data->item.summary.type);
}

static inline void
summary_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_SUMMARY) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUMMARY;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->item.summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
content_start(struct parser_data *data, const XML_Char **atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	data->atom10_pos |= ATOM10_CONTENT;
	copy_type_of_text_construct(data, atts, data->item.content.type);
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
	if (cpyss(data->item.content.value, data->value) == false) {
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
		return; // Ignore feed id.
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
	} else {
		data->feed.update_time = parse_date_rfc3339(data->value);
	}
}

static inline void
author_start(struct parser_data *data)
{
	data->atom10_pos |= ATOM10_AUTHOR;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (expand_person_list_by_one_element(&data->item.authors) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global author element
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
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
			if (add_name_to_last_person(&data->item.authors, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			// TODO: Global author element
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
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
			if (add_link_to_last_person(&data->item.authors, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			// TODO: Global author element
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
	if ((data->atom10_pos & ATOM10_AUTHOR) != 0) {
		if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
			if (add_email_to_last_person(&data->item.authors, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			// TODO: Global author element
		}
	}
}

static inline void
category_start(struct parser_data *data, const XML_Char **atts)
{
	const char *category = get_value_of_attribute_key(atts, "term");
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (add_category_to_item_bucket(&data->item, category, strlen(category)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global categories
	}
}

static inline void
subtitle_start(struct parser_data *data, const XML_Char **atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	data->atom10_pos |= ATOM10_SUBTITLE;
	copy_type_of_text_construct(data, atts, data->feed.summary.type);
}

static inline void
subtitle_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_SUBTITLE) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUBTITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed.summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
generator_start(struct parser_data *data, const XML_Char **atts)
{
	if ((data->atom10_pos & ATOM10_GENERATOR) != 0) {
		return;
	}
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	data->atom10_pos |= ATOM10_GENERATOR;
	const char *version = get_value_of_attribute_key(atts, "version");
	if (version != NULL) {
		if (cpyas(data->feed.generator.version, version, strlen(version)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *uri = get_value_of_attribute_key(atts, "uri");
	if (uri != NULL) {
		if (cpyas(data->feed.generator.url, uri, strlen(uri)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
generator_end(struct parser_data *data)
{
	if ((data->atom10_pos & ATOM10_GENERATOR) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_GENERATOR;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed.generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_atom10_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	     if (strcmp(name, "entry")       == 0) { entry_start(data);           }
	else if (strcmp(name, "id")          == 0) { id_start(data);              }
	else if (strcmp(name, "title")       == 0) { title_start(data, atts);     }
	else if (strcmp(name, "link")        == 0) { link_start(data, atts);      }
	else if (strcmp(name, "summary")     == 0) { summary_start(data, atts);   }
	else if (strcmp(name, "content")     == 0) { content_start(data, atts);   }
	else if (strcmp(name, "published")   == 0) { published_start(data);       }
	else if (strcmp(name, "updated")     == 0) { updated_start(data);         }
	else if (strcmp(name, "author")      == 0) { author_start(data);          }
	else if (strcmp(name, "contributor") == 0) { author_start(data);          }
	else if (strcmp(name, "name")        == 0) { name_start(data);            }
	else if (strcmp(name, "uri")         == 0) { uri_start(data);             }
	else if (strcmp(name, "email")       == 0) { email_start(data);           }
	else if (strcmp(name, "category")    == 0) { category_start(data, atts);  }
	else if (strcmp(name, "subtitle")    == 0) { subtitle_start(data, atts);  }
	else if (strcmp(name, "generator")   == 0) { generator_start(data, atts); }
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
	else if (strcmp(name, "generator")   == 0) { generator_end(data); }
	// In Atom 1.0 link tag is a self-closing tag.
	//else if (strcmp(name, "link")     == 0) {                       }
	// In Atom 1.0 category tag is a self-closing tag.
	//else if (strcmp(name, "category") == 0) {                       }
}
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM10
