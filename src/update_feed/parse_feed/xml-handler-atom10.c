#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static inline void
copy_type_of_text_construct(struct xml_data *data, const TidyAttr atts, struct string *dest)
{
	// Valid Atom 1.0 text construct types are "text", "html" and "xhtml".
	// If the "type" attribute is not provided, then we MUST assume
	// that data is of "text" type (or "text/plain" in terms of MIME standard).
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if ((strcmp(type, "html") == 0) || (strcmp(type, "xhtml") == 0)) {
			if (cpyas(dest, "text/html", 9) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
			return;
		}
	}
	if (cpyas(dest, "text/plain", 10) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
entry_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_ENTRY;
	prepend_item(&data->feed->item);
}

static inline void
entry_end(struct xml_data *data)
{
	data->atom10_pos &= ~ATOM10_ENTRY;
}

static inline void
title_start(struct xml_data *data, const TidyAttr atts)
{
	data->atom10_pos |= ATOM10_TITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		copy_type_of_text_construct(data, atts, data->feed->item->title.type);
	} else {
		copy_type_of_text_construct(data, atts, data->feed->title.type);
	}
}

static inline void
title_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_TITLE) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_TITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
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
link_start(struct xml_data *data, const TidyAttr atts)
{
	const char *href = get_value_of_attribute_key(atts, "href");
	if (href == NULL) {
		// In Atom 1.0 links href attribute MUST be set.
		return;
	}
	const char *rel = get_value_of_attribute_key(atts, "rel");
	if ((rel != NULL) && (strcmp(rel, "self") == 0)) {
		// Ignore links to feed itself.
		return;
	}
	if ((rel == NULL) || (strcmp(rel, "alternate") == 0)) {
		// Default value of rel is alternate.
		if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
			if (cpyas(data->feed->item->url, href, strlen(href)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		} else {
			if (cpyas(data->feed->url, href, strlen(href)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		const char *type = get_value_of_attribute_key(atts, "type");
		const char *length = get_value_of_attribute_key(atts, "length");
		if (prepend_link(&data->feed->item->attachment) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyas(data->feed->item->attachment->url, href, strlen(href)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (type != NULL) {
			if (cpyas(data->feed->item->attachment->type, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (length != NULL) {
			data->feed->item->attachment->size = convert_string_to_size_t_or_zero(length);
		}
	}
}

static inline void
summary_start(struct xml_data *data, const TidyAttr atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	data->atom10_pos |= ATOM10_SUMMARY;
	copy_type_of_text_construct(data, atts, data->feed->item->summary.type);
}

static inline void
summary_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_SUMMARY) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUMMARY;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->feed->item->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
content_start(struct xml_data *data, const TidyAttr atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	data->atom10_pos |= ATOM10_CONTENT;
	copy_type_of_text_construct(data, atts, data->feed->item->content.type);
}

static inline void
content_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_CONTENT) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_CONTENT;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return;
	}
	if (cpyss(data->feed->item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
id_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_ID;
}

static inline void
id_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_ID) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_ID;
	if ((data->atom10_pos & ATOM10_ENTRY) == 0) {
		return; // Ignore feed id.
	}
	if (cpyss(data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
published_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_PUBLISHED;
}

static inline void
published_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_PUBLISHED) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_PUBLISHED;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		data->feed->item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static inline void
updated_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_UPDATED;
}

static inline void
updated_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_UPDATED) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_UPDATED;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		data->feed->item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed->update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static inline void
author_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_AUTHOR;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (prepend_person(&data->feed->item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global author element
	}
}

static inline void
author_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_AUTHOR;
}

static inline void
name_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_NAME;
}

static inline void
name_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_NAME) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_NAME;
	if ((data->atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global author element
	}
}

static inline void
uri_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_URI;
}

static inline void
uri_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_URI) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_URI;
	if ((data->atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global author element
	}
}

static inline void
email_start(struct xml_data *data)
{
	data->atom10_pos |= ATOM10_EMAIL;
}

static inline void
email_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_EMAIL) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_EMAIL;
	if ((data->atom10_pos & ATOM10_AUTHOR) == 0) {
		return;
	}
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (cpyss(data->feed->item->author->email, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO: Global author element
	}
}

static inline void
category_start(struct xml_data *data, const TidyAttr atts)
{
	const char *term = get_value_of_attribute_key(atts, "term");
	if (term == NULL) {
		// Atom 1.0 says that every category MUST have term attribute.
		return;
	}
	const char *label = get_value_of_attribute_key(atts, "label");
	const char *scheme = get_value_of_attribute_key(atts, "scheme");
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		if (prepend_category(&data->feed->item->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyas(data->feed->item->category->term, term, strlen(term)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (label != NULL) {
			if (cpyas(data->feed->item->category->label, label, strlen(label)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (scheme != NULL) {
			if (cpyas(data->feed->item->category->scheme, scheme, strlen(scheme)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (prepend_category(&data->feed->category) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyas(data->feed->category->term, term, strlen(term)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (label != NULL) {
			if (cpyas(data->feed->category->label, label, strlen(label)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
		if (scheme != NULL) {
			if (cpyas(data->feed->category->scheme, scheme, strlen(scheme)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static inline void
subtitle_start(struct xml_data *data, const TidyAttr atts)
{
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	data->atom10_pos |= ATOM10_SUBTITLE;
	copy_type_of_text_construct(data, atts, data->feed->summary.type);
}

static inline void
subtitle_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_SUBTITLE) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_SUBTITLE;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
generator_start(struct xml_data *data, const TidyAttr atts)
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
		if (cpyas(data->feed->generator.version, version, strlen(version)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *uri = get_value_of_attribute_key(atts, "uri");
	if (uri != NULL) {
		if (cpyas(data->feed->generator.url, uri, strlen(uri)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
generator_end(struct xml_data *data)
{
	if ((data->atom10_pos & ATOM10_GENERATOR) == 0) {
		return;
	}
	data->atom10_pos &= ~ATOM10_GENERATOR;
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed->generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_atom10_element_start(struct xml_data *data, const char *name, const TidyAttr atts)
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
parse_atom10_element_end(struct xml_data *data, const char *name)
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
