#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20210303084351/https://pythonhosted.org/feedparser/annotated-atom03.html
// https://web.archive.org/web/20210417183310/http://rakaz.nl/2005/07/moving-from-atom-03-to-10.html

// Note to the future.
// Atom 0.3 does not have category element.

static inline void
entry_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_ENTRY;
}

static inline void
entry_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_ENTRY;
	insert_item(data->feed_url, &data->item);
	empty_item_bucket(&data->item);
}

static inline void
title_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_TITLE) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_TITLE;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
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

// Atom 0.3 link element does not have length attribute.
static inline void
link_start(struct parser_data *data, const XML_Char **atts)
{
	const char *href = NULL, *type = NULL;
	bool this_is_enclosure = false;
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], "href") == 0) {
			href = atts[i + 1];
		} else if (strcmp(atts[i], "type") == 0) {
			type = atts[i + 1];
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
	} else {
		if (href != NULL) {
			if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
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
	data->atom03_pos |= ATOM03_SUMMARY;
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom03_pos & ATOM03_ENTRY) != 0)) {
		if (cpyas(data->item.summary.type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
summary_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_SUMMARY) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_SUMMARY;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		if (cpyss(data->item.summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
content_start(struct parser_data *data, const XML_Char **atts)
{
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom03_pos & ATOM03_ENTRY) != 0)) {
		if (cpyas(data->item.content.type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	data->atom03_pos |= ATOM03_CONTENT;
}

static inline void
content_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_CONTENT) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_CONTENT;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
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
	data->atom03_pos |= ATOM03_ID;
}

static inline void
id_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_ID) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_ID;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// In Atom 0.3 feed can have unique id, but who needs it?
		return;
	}
	if (cpyss(data->item.guid, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
issued_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_ISSUED;
}

static inline void
issued_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_ISSUED) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_ISSUED;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 feed can have issued date but who needs it?
		return;
	}
	data->item.pubdate = parse_date_rfc3339(data->value);
}

static inline void
modified_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_MODIFIED;
}

static inline void
modified_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_MODIFIED) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_MODIFIED;
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		data->item.upddate = parse_date_rfc3339(data->value);
	} else {
		data->feed.update_time = parse_date_rfc3339(data->value);
	}
}

static inline void
author_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_AUTHOR;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed must have at least one author, but who needs it?
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
	if ((data->atom03_pos & ATOM03_AUTHOR) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_AUTHOR;
}

static inline void
name_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_NAME;
}

static inline void
name_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_NAME) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_NAME;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) != 0) {
		if (add_name_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
url_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_URL;
}

static inline void
url_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_URL) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_URL;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) != 0) {
		if (add_link_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
email_start(struct parser_data *data)
{
	data->atom03_pos |= ATOM03_EMAIL;
}

static inline void
email_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_EMAIL) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_EMAIL;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		// Atom 0.3 says that feed can have global author, but who needs it?
		return;
	}
	if ((data->atom03_pos & ATOM03_AUTHOR) != 0) {
		if (add_email_to_last_person(&data->item.authors, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
tagline_start(struct parser_data *data, const XML_Char **atts)
{
	data->atom03_pos |= ATOM03_TAGLINE;
	const char *type_str = get_value_of_attribute_key(atts, "type");
	if ((type_str != NULL) && ((data->atom03_pos & ATOM03_ENTRY) == 0)) {
		if (cpyas(data->feed.summary.type, type_str, strlen(type_str)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
tagline_end(struct parser_data *data)
{
	if ((data->atom03_pos & ATOM03_TAGLINE) == 0) {
		return;
	}
	data->atom03_pos &= ~ATOM03_TAGLINE;
	if ((data->atom03_pos & ATOM03_ENTRY) == 0) {
		if (cpyss(data->feed.summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

void
parse_atom03_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	     if (strcmp(name, "entry")       == 0) { entry_start(data);         }
	else if (strcmp(name, "id")          == 0) { id_start(data);            }
	else if (strcmp(name, "title")       == 0) { title_start(data);         }
	else if (strcmp(name, "link")        == 0) { link_start(data, atts);    }
	else if (strcmp(name, "summary")     == 0) { summary_start(data, atts); }
	else if (strcmp(name, "content")     == 0) { content_start(data, atts); }
	else if (strcmp(name, "issued")      == 0) { issued_start(data);        }
	else if (strcmp(name, "modified")    == 0) { modified_start(data);      }
	else if (strcmp(name, "author")      == 0) { author_start(data);        }
	else if (strcmp(name, "contributor") == 0) { author_start(data);        }
	else if (strcmp(name, "name")        == 0) { name_start(data);          }
	else if (strcmp(name, "url")         == 0) { url_start(data);           }
	else if (strcmp(name, "email")       == 0) { email_start(data);         }
	else if (strcmp(name, "tagline")     == 0) { tagline_start(data, atts); }
}

void
parse_atom03_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "entry")       == 0) { entry_end(data);    }
	else if (strcmp(name, "id")          == 0) { id_end(data);       }
	else if (strcmp(name, "title")       == 0) { title_end(data);    }
	else if (strcmp(name, "summary")     == 0) { summary_end(data);  }
	else if (strcmp(name, "content")     == 0) { content_end(data);  }
	else if (strcmp(name, "issued")      == 0) { issued_end(data);   }
	else if (strcmp(name, "modified")    == 0) { modified_end(data); }
	else if (strcmp(name, "author")      == 0) { author_end(data);   }
	else if (strcmp(name, "contributor") == 0) { author_end(data);   }
	else if (strcmp(name, "name")        == 0) { name_end(data);     }
	else if (strcmp(name, "url")         == 0) { url_end(data);      }
	else if (strcmp(name, "email")       == 0) { email_end(data);    }
	else if (strcmp(name, "tagline")     == 0) { tagline_end(data);  }
	// In Atom 0.3 link tag is a self-closing tag.
	//else if (strcmp(name, "link")      == 0) {                     }
}
#endif // FEEDEATER_FORMAT_SUPPORT_ATOM03
