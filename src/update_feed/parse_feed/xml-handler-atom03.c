#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

// https://web.archive.org/web/20210303084351/https://pythonhosted.org/feedparser/annotated-atom03.html
// https://web.archive.org/web/20210417183310/http://rakaz.nl/2005/07/moving-from-atom-03-to-10.html

// Note to the future.
// Atom 0.3 does not have category element.

static inline void
copy_type_of_text_construct(struct xml_data *data, const TidyAttr atts, struct string *dest)
{
	// Atom 0.3 text construct types are fully compliant with MIME standard.
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if (cpyas(dest, type, strlen(type)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
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
	data->atom03_pos |= ATOM03_ENTRY;
	prepend_item(&data->feed->item);
}

static inline void
entry_end(struct xml_data *data)
{
	data->atom03_pos &= ~ATOM03_ENTRY;
}

static inline void
title_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_TITLE;
}

static inline void
title_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_TITLE) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_TITLE;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
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
	// Atom 0.3 link element does not have length and hreflang attributes.
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
		if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
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
	} else if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		if (prepend_link(&data->feed->item->attachment) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyas(data->feed->item->attachment->url, href, strlen(href)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		const char *type = get_value_of_attribute_key(atts, "type");
		if (type != NULL) {
			if (cpyas(data->feed->item->attachment->type, type, strlen(type)) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	}
}

static inline void
summary_start(struct xml_data *data, const TidyAttr atts)
{
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		return;
	}
	data->atom03_pos |= ATOM03_SUMMARY;
	copy_type_of_text_construct(data, atts, data->feed->item->summary.type);
}

static inline void
summary_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_SUMMARY) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_SUMMARY;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
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
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		return;
	}
	data->atom03_pos |= ATOM03_CONTENT;
	copy_type_of_text_construct(data, atts, data->feed->item->content.type);
}

static inline void
content_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_CONTENT) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_CONTENT;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
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
	data->atom03_pos |= ATOM03_ID;
}

static inline void
id_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_ID) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_ID;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// In Atom 0.3 feed can have unique id, but who needs it?
		return;
	}
	if (cpyss(data->feed->item->guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
issued_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_ISSUED;
}

static inline void
issued_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_ISSUED) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_ISSUED;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 feed can have issued date but who needs it?
		return;
	}
	data->feed->item->pubdate = parse_date_rfc3339(data->value->ptr, data->value->len);
}

static inline void
modified_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_MODIFIED;
}

static inline void
modified_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_MODIFIED) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_MODIFIED;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		data->feed->item->upddate = parse_date_rfc3339(data->value->ptr, data->value->len);
	} else {
		data->feed->update_date = parse_date_rfc3339(data->value->ptr, data->value->len);
	}
}

static inline void
author_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_AUTHOR;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed must have at least one author, but who needs it?
		return;
	}
	if (prepend_person(&data->feed->item->author) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
author_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_AUTHOR) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_AUTHOR;
}

static inline void
name_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_NAME;
}

static inline void
name_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_NAME) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_NAME;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) == 0) {
		return;
	}
	if (cpyss(data->feed->item->author->name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
url_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_URL;
}

static inline void
url_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_URL) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_URL;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) == 0) {
		return;
	}
	if (cpyss(data->feed->item->author->url, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
email_start(struct xml_data *data)
{
	data->atom03_pos |= ATOM03_EMAIL;
}

static inline void
email_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_EMAIL) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_EMAIL;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) == 0) {
		return;
	}
	if (cpyss(data->feed->item->author->email, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
tagline_start(struct xml_data *data, const TidyAttr atts)
{
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		return;
	}
	data->atom03_pos |= ATOM03_TAGLINE;
	copy_type_of_text_construct(data, atts, data->feed->summary.type);
}

static inline void
tagline_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_TAGLINE) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_TAGLINE;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
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
	if ((data->atom03_pos & ATOM03_GENERATOR) != 0) {
		return;
	}
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		return;
	}
	data->atom03_pos |= ATOM03_GENERATOR;
	const char *version = get_value_of_attribute_key(atts, "version");
	if (version != NULL) {
		if (cpyas(data->feed->generator.version, version, strlen(version)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *url = get_value_of_attribute_key(atts, "url");
	if (url != NULL) {
		if (cpyas(data->feed->generator.url, url, strlen(url)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
generator_end(struct xml_data *data)
{
	if ((data->atom03_pos & ATOM03_GENERATOR) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_GENERATOR;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		return;
	}
	if (cpyss(data->feed->generator.name, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_atom03_element_start(struct xml_data *data, const char *name, const TidyAttr atts)
{
	     if (strcmp(name, "entry")       == 0) { entry_start(data);           }
	else if (strcmp(name, "id")          == 0) { id_start(data);              }
	else if (strcmp(name, "title")       == 0) { title_start(data);           }
	else if (strcmp(name, "link")        == 0) { link_start(data, atts);      }
	else if (strcmp(name, "summary")     == 0) { summary_start(data, atts);   }
	else if (strcmp(name, "content")     == 0) { content_start(data, atts);   }
	else if (strcmp(name, "issued")      == 0) { issued_start(data);          }
	else if (strcmp(name, "modified")    == 0) { modified_start(data);        }
	else if (strcmp(name, "author")      == 0) { author_start(data);          }
	else if (strcmp(name, "contributor") == 0) { author_start(data);          }
	else if (strcmp(name, "name")        == 0) { name_start(data);            }
	else if (strcmp(name, "url")         == 0) { url_start(data);             }
	else if (strcmp(name, "email")       == 0) { email_start(data);           }
	else if (strcmp(name, "tagline")     == 0) { tagline_start(data, atts);   }
	else if (strcmp(name, "generator")   == 0) { generator_start(data, atts); }
}

void
parse_atom03_element_end(struct xml_data *data, const char *name)
{
	     if (strcmp(name, "entry")       == 0) { entry_end(data);     }
	else if (strcmp(name, "id")          == 0) { id_end(data);        }
	else if (strcmp(name, "title")       == 0) { title_end(data);     }
	else if (strcmp(name, "summary")     == 0) { summary_end(data);   }
	else if (strcmp(name, "content")     == 0) { content_end(data);   }
	else if (strcmp(name, "issued")      == 0) { issued_end(data);    }
	else if (strcmp(name, "modified")    == 0) { modified_end(data);  }
	else if (strcmp(name, "author")      == 0) { author_end(data);    }
	else if (strcmp(name, "contributor") == 0) { author_end(data);    }
	else if (strcmp(name, "name")        == 0) { name_end(data);      }
	else if (strcmp(name, "url")         == 0) { url_end(data);       }
	else if (strcmp(name, "email")       == 0) { email_end(data);     }
	else if (strcmp(name, "tagline")     == 0) { tagline_end(data);   }
	else if (strcmp(name, "generator")   == 0) { generator_end(data); }
	// In Atom 0.3 link tag is a self-closing tag.
	//else if (strcmp(name, "link")      == 0) {                     }
}
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM03
