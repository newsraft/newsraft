#include <string.h>
#include "update_feed/parse_json/parse_json_feed.h"

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
		if (crtas_or_cpyas(&data->feed.title, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "description") == 0) {
		if (serialize_caret(&data->feed.content) == false) {
			return false;
		}
		if (serialize_array(&data->feed.content, "text", 4, val, len) == false) {
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
		if (crtas_or_cpyas(&data->feed.item->title, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "content_html") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return false;
		}
		if (serialize_array(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return false;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "content_text") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return false;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "summary") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return false;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "date_published") == 0) {
		data->feed.item->publication_date = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "date_modified") == 0) {
		data->feed.item->update_date = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "external_url") == 0) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return false;
		}
		if (serialize_array(&data->feed.item->attachments, "url", 3, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
person_string_handler(struct string **dest, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "name") == 0) {
		if (serialize_array(dest, "name", 4, val, len) == false) {
			return false;
		}
	} else if (strcmp(key->ptr, "url") == 0) {
		if (serialize_array(dest, "url", 3, val, len) == false) {
			return false;
		}
	} else if (strcmp(key->ptr, "avatar") == 0) {
		if (serialize_array(dest, "avatar", 6, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
item_attachment_string_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (strcmp(data->text->ptr, "url") == 0) {
		if (serialize_array(&data->feed.item->attachments, "url", 3, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "mime_type") == 0) {
		if (serialize_array(&data->feed.item->attachments, "type", 4, val, len) == false) {
			return false;
		}
	}
	return true;
}

static inline bool
item_attachment_number_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (strcmp(data->text->ptr, "size_in_bytes") == 0) {
		if (serialize_array(&data->feed.item->attachments, "size", 4, val, len) == false) {
			return false;
		}
	} else if (strcmp(data->text->ptr, "duration_in_seconds") == 0) {
		if (serialize_array(&data->feed.item->attachments, "duration", 8, val, len) == false) {
			return false;
		}
	}
	return true;
}

static int
null_handler(void *ctx)
{
	(void)ctx;
	// JSON Feed doesn't have any NULL.
	WARN("Stumbled upon null.");
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
		if (item_attachment_number_handler(data, val, len) == false) {
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
			if (item_string_handler(data, (char *)val, len) == false) {
				return 0;
			}
		} else if (data->path[0] == JSON_ARRAY_AUTHORS) {
			if (person_string_handler(&data->feed.persons, data->text, (char *)val, len) == false) {
				return 0;
			}
		}
	} else if (data->depth == 0) {
		if (feed_string_handler(data, (char *)val, len) == false) {
			return 0;
		}
	} else if (data->depth == 2) {
		if ((data->path[0] == JSON_ARRAY_ITEMS) && (data->feed.item != NULL)) {
			if (data->path[1] == JSON_ARRAY_AUTHORS) {
				if (person_string_handler(&data->feed.item->persons, data->text, (char *)val, len) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_ATTACHMENTS) {
				if (item_attachment_string_handler(data, (char *)val, len) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_TAGS) {
				if (serialize_caret(&data->feed.item->extras) == false) {
					return 0;
				}
				if (serialize_array(&data->feed.item->extras, "category", 8, (char *)val, len) == false) {
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
	INFO("Stumbled upon the beginning of an object.");
	if (data->depth == 1) {
		if (data->path[0] == JSON_ARRAY_ITEMS) {
			prepend_item(&data->feed.item);
		} else if (data->path[0] == JSON_ARRAY_AUTHORS) {
			if (serialize_caret(&data->feed.persons) == false) {
				return 0;
			}
			if (serialize_array(&data->feed.persons, "type", 4, "author", 6) == false) {
				return 0;
			}
		}
	} else if (data->depth == 2) {
		if ((data->path[0] == JSON_ARRAY_ITEMS) && (data->feed.item != NULL)) {
			if (data->path[1] == JSON_ARRAY_AUTHORS) {
				if (serialize_caret(&data->feed.item->persons) == false) {
					return 0;
				}
				if (serialize_array(&data->feed.item->persons, "type", 4, "author", 6) == false) {
					return 0;
				}
			} else if (data->path[1] == JSON_ARRAY_ATTACHMENTS) {
				if (serialize_caret(&data->feed.item->attachments) == false) {
					return 0;
				}
			}
		}
	}
	return 1;
}

static int
map_key_handler(void *ctx, const unsigned char *key, size_t key_len)
{
	struct stream_callback_data *data = ctx;
	cpyas(data->text, (char *)key, key_len);
	INFO("Stumbled upon key: %s", data->text->ptr);
	return 1;
}

static int
end_map_handler(void *ctx)
{
	(void)ctx;
	INFO("Stumbled upon the end of an object.");
	return 1;
}

static int
start_array_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon the beginning of an array.");
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
	INFO("Stumbled upon the end of an array.");
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
