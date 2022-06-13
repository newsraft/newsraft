#include <string.h>
#include "update_feed/parse_feed/json/parse_json_feed.h"

enum json_array {
	JSON_ARRAY_UNKNOWN,
	JSON_ARRAY_ITEMS,
	JSON_ARRAY_ATTACHMENTS,
	JSON_ARRAY_AUTHORS,
	JSON_ARRAY_TAGS,
};

static inline bool
feed_string_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (strcmp(data->text->ptr, "home_page_url") == 0) {
		if (crtas_or_cpyas(&data->feed.url, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "title") == 0) {
		if (crtas_or_cpyas(&data->feed.title.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.title.type, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "description") == 0) {
		if (crtas_or_cpyas(&data->feed.summary.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.summary.type, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "language") == 0) {
		if (crtas_or_cpyas(&data->feed.language, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
item_string_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (data->feed.item == NULL) {
		return true;
	}
	if (strcmp(data->text->ptr, "id") == 0) {
		if (crtas_or_cpyas(&data->feed.item->guid, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "url") == 0) {
		if (crtas_or_cpyas(&data->feed.item->url, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "title") == 0) {
		if (crtas_or_cpyas(&data->feed.item->title.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "content_html") == 0) {
		if (crtas_or_cpyas(&data->feed.item->content.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.item->content.type, "text/html", 9) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "content_text") == 0) {
		if (crtas_or_cpyas(&data->feed.item->content.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.item->content.type, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "summary") == 0) {
		if (crtas_or_cpyas(&data->feed.item->summary.value, val, len) == false) {
			return false;
		}
		if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "date_published") == 0) {
		data->feed.item->pubdate = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "date_modified") == 0) {
		data->feed.item->upddate = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "external_url") == 0) {
		if (cat_array_to_serialization(&data->feed.item->attachments, "url", 3, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "language") == 0) {
		if (crtas_or_cpyas(&data->feed.item->language, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
person_string_handler(struct getfeed_person *person, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "name") == 0) {
		if (crtas_or_cpyas(&person->name, val, len) == false) {
			return false;
		}
	} else if (strcmp(key->ptr, "url") == 0) {
		if (crtas_or_cpyas(&person->url, val, len) == false) {
			return false;
		}
	} else if (strcmp(key->ptr, "avatar") == 0) {
		// avatar...
	}
	return true;
}

static inline bool
attachment_string_handler(struct getfeed_link *link, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "url") == 0) {
		if (crtas_or_cpyas(&link->url, val, len) == false) {
			return false;
		}
	} else if (strcmp(key->ptr, "mime_type") == 0) {
		if (crtas_or_cpyas(&link->type, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
attachment_number_handler(struct getfeed_link *link, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "size_in_bytes") == 0) {
		// To convert a string to a number, it must be null-terminated;
		// which val is not - so we have to create a null-terminated copy
		// of this string.
		char *tmp = malloc(sizeof(char) * (len + 1));
		if (tmp == NULL) {
			return false;
		}
		memcpy(tmp, val, sizeof(char) * len);
		tmp[len] = '\0';
		link->size = convert_string_to_size_t_or_zero(tmp);
		free(tmp);
	} else if (strcmp(key->ptr, "duration_in_seconds") == 0) {
		// We explained these tricks above.
		char *tmp = malloc(sizeof(char) * (len + 1));
		if (tmp == NULL) {
			return false;
		}
		memcpy(tmp, val, sizeof(char) * len);
		tmp[len] = '\0';
		link->duration = convert_string_to_size_t_or_zero(tmp);
		free(tmp);
	}
	return true;
}

static int
null_handler(void *ctx)
{
	(void)ctx;
	// JSON Feed doesn't have any NULL.
	WARN("Stumbled upon NULL.");
	return 1;
}

static int
boolean_handler(void *ctx, int value)
{
	(void)ctx;
	// TODO expired
	INFO("Stumbled upon boolean: %d", value);
	return 1;
}

static int
integer_handler(void *ctx, long long value)
{
	(void)ctx;
	// JSON Feed doesn't have any integers.
	WARN("Stumbled upon integer: %lld", value);
	return 1;
}

static int
double_handler(void *ctx, double value)
{
	(void)ctx;
	// JSON Feed doesn't have any doubles.
	WARN("Stumbled upon double: %lf", value);
	return 1;
}

static int
number_handler(void *ctx, const char *val, size_t len)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon number.");
	if ((data->depth == 2)
			&& (data->path[0] == JSON_ARRAY_ITEMS)
			&& (data->path[1] == JSON_ARRAY_ATTACHMENTS)
			&& (data->feed.item != NULL))
	{
		if (attachment_number_handler(&data->feed.temp.attachment, data->text, val, len) == false) {
			return 0;
		}
	}
	return 1;
}

static int
string_handler(void *ctx, const unsigned char *val, size_t len)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon string.");
	if (data->depth == 1) {
		if (data->path[0] == JSON_ARRAY_ITEMS) {
			if (item_string_handler(data, (const char *)val, len) == false) {
				return 0;
			}
		} else if (data->path[0] == JSON_ARRAY_AUTHORS) {
			if (person_string_handler(&data->feed.temp.author, data->text, (const char *)val, len) == false) {
				return 0;
			}
		}
	} else if (data->depth == 0) {
		if (feed_string_handler(data, (const char *)val, len) == false) {
			return 0;
		}
	} else if (data->depth == 2) {
		if ((data->path[0] == JSON_ARRAY_ITEMS) && (data->feed.item != NULL)) {
			if (data->path[1] == JSON_ARRAY_AUTHORS) {
				if (person_string_handler(&data->feed.temp.author, data->text, (const char *)val, len) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_ATTACHMENTS) {
				if (attachment_string_handler(&data->feed.temp.attachment, data->text, (const char *)val, len) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_TAGS) {
				if (cat_array_to_serialization(&data->feed.item->categories, "term", 4, (const char *)val, len) == false) {
					return 0;
				}
			}
		}
	}
	return 1;
}

static int
start_map_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon object start.");
	if (data->depth == 1) {
		if (data->path[0] == JSON_ARRAY_ITEMS) {
			prepend_item(&data->feed.item);
		} else if (data->path[0] == JSON_ARRAY_AUTHORS) {
			empty_person(&data->feed.temp);
		}
	} else if (data->depth == 2) {
		if ((data->path[0] == JSON_ARRAY_ITEMS) && (data->feed.item != NULL)) {
			if (data->path[1] == JSON_ARRAY_AUTHORS) {
				empty_person(&data->feed.temp);
			} else if (data->path[1] == JSON_ARRAY_ATTACHMENTS) {
				empty_link(&data->feed.temp);
			}
		}
	}
	return 1;
}

static int
map_key_handler(void *ctx, const unsigned char *key, size_t key_len)
{
	struct stream_callback_data *data = ctx;
	cpyas(data->text, (const char *)key, key_len);
	INFO("Stumbled upon key: %s", data->text->ptr);
	return 1;
}

static int
end_map_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon object end.");
	if (data->depth == 1) {
		if (data->path[0] == JSON_ARRAY_AUTHORS) {
			if (serialize_person(&data->feed.temp, &data->feed.authors) == false) {
				return 0;
			}
		}
	} else if (data->depth == 2) {
		if ((data->path[0] == JSON_ARRAY_ITEMS) && (data->feed.item != NULL)) {
			if (data->path[1] == JSON_ARRAY_AUTHORS) {
				if (serialize_person(&data->feed.temp, &data->feed.item->authors) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_ATTACHMENTS) {
				if (serialize_link(&data->feed.temp, &data->feed.item->attachments) == false) {
					return 0;
				}
			}
		}
	}
	return 1;
}

static int
start_array_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon array start.");
	if (strcmp(data->text->ptr, "items") == 0) {
		data->path[data->depth] = JSON_ARRAY_ITEMS;
	} else if (strcmp(data->text->ptr, "attachments") == 0) {
		data->path[data->depth] = JSON_ARRAY_ATTACHMENTS;
	} else if (strcmp(data->text->ptr, "authors") == 0) {
		data->path[data->depth] = JSON_ARRAY_AUTHORS;
	} else if (strcmp(data->text->ptr, "tags") == 0) {
		data->path[data->depth] = JSON_ARRAY_TAGS;
	} else {
		data->path[data->depth] = JSON_ARRAY_UNKNOWN;
	}
	data->depth += 1;
	return 1;
}

static int
end_array_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon array end.");
	if (data->depth > 0) {
		data->depth -= 1;
	}
	return 1;
}

static const yajl_callbacks callbacks = {
	&null_handler,
	&boolean_handler,
	&integer_handler,
	&double_handler,
	&number_handler,
	&string_handler,
	&start_map_handler,
	&map_key_handler,
	&end_map_handler,
	&start_array_handler,
	&end_array_handler
};

bool
engage_json_parser(struct stream_callback_data *data)
{
	data->text = crtes();
	if (data->text == NULL) {
		return false;
	}
	data->json_parser = yajl_alloc(&callbacks, NULL, data);
	if (data->json_parser == NULL) {
		free_string(data->text);
		return false;
	}
	data->depth = 0;
	return true;
}

void
free_json_parser(struct stream_callback_data *data)
{
	free_string(data->text);
	yajl_complete_parse(data->json_parser); // final call
	yajl_free(data->json_parser);
}
