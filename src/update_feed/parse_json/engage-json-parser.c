#include <string.h>
#include "update_feed/update_feed.h"

enum json_array {
	JSON_OBJECT_UNKNOWN,
	JSON_OBJECT_ITEM,
	JSON_OBJECT_ATTACHMENT,
	JSON_OBJECT_AUTHOR,
	JSON_ARRAY_UNKNOWN,
	JSON_ARRAY_ITEMS,
	JSON_ARRAY_ATTACHMENTS,
	JSON_ARRAY_AUTHORS,
	JSON_ARRAY_TAGS,
};

#define ISARRAY(A) ((A) >= JSON_ARRAY_UNKNOWN)

static inline bool
we_are_inside_item(struct stream_callback_data *data)
{
	for (uint8_t i = 1; i < data->depth; ++i) {
		if (data->path[i] == JSON_OBJECT_ITEM) {
			return true;
		}
	}
	return false;
}

static inline int
feed_string_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (strcmp(data->text->ptr, "home_page_url") == 0) {
		if (crtas_or_cpyas(&data->feed.url, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "title") == 0) {
		if (crtas_or_cpyas(&data->feed.title, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "description") == 0) {
		if (serialize_caret(&data->feed.content) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.content, "text", 4, val, len) == false) {
			return 0;
		}
	}
	return 1;
}

static inline int
item_string_handler(struct stream_callback_data *data, const char *val, size_t len)
{
	if (data->feed.item == NULL) {
		return 1;
	}
	if (strcmp(data->text->ptr, "id") == 0) {
		if (crtas_or_cpyas(&data->feed.item->guid, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "url") == 0) {
		if (crtas_or_cpyas(&data->feed.item->url, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "title") == 0) {
		if (crtas_or_cpyas(&data->feed.item->title, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "content_html") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "content_text") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "summary") == 0) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.item->content, "text", 4, val, len) == false) {
			return 0;
		}
	} else if (strcmp(data->text->ptr, "date_published") == 0) {
		data->feed.item->publication_date = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "date_modified") == 0) {
		data->feed.item->update_date = parse_date_rfc3339(val, len);
	} else if (strcmp(data->text->ptr, "external_url") == 0) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.item->attachments, "url", 3, val, len) == false) {
			return 0;
		}
	}
	return 1;
}

// Note to the future.
// Person structure in the JSON Feed have an avatar object. We ignore it just
// like we ignore thumbnails, icons and other cosmetic stuff. Also, JSON Feed
// doesn't provide email objects. Gosh...

static inline int
person_string_handler(struct string **dest, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "name") == 0) {
		if (serialize_array(dest, "name", 4, val, len) == false) {
			return 0;
		}
	} else if (strcmp(key->ptr, "url") == 0) {
		if (serialize_array(dest, "url", 3, val, len) == false) {
			return 0;
		}
	}
	return 1;
}

static inline int
attachment_string_handler(struct string **dest, const struct string *key, const char *val, size_t len)
{
	if (strcmp(key->ptr, "url") == 0) {
		if (serialize_array(dest, "url", 3, val, len) == false) {
			return 0;
		}
	} else if (strcmp(key->ptr, "mime_type") == 0) {
		if (serialize_array(dest, "type", 4, val, len) == false) {
			return 0;
		}
	}
	return 1;
}

// Note to the future.
// There are two token handlers which we could've set to NULL in yajl_callbacks
// structure because they have no use in actual JSON Feed, but we have to create
// these dummy functions to return 1 (no error) anyway because when parser
// doesn't find appropriate handler for token it returns error and we're screwed.

static int
null_handler(void *ctx)
{
	(void)ctx;
	return 1; // Null handler is redundant because the spec doesn't mention it.
}

static int
boolean_handler(void *ctx, int val)
{
	(void)ctx;
	(void)val;
	return 1; // Boolean handler might be used for expired object, but it's useless.
}

static int
number_handler(void *ctx, const char *val, size_t len)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon number.");
	if ((we_are_inside_item(data) == true) && (data->path[data->depth] == JSON_OBJECT_ATTACHMENT)) {
		if (strcmp(data->text->ptr, "size_in_bytes") == 0) {
			if (serialize_array(&data->feed.item->attachments, "size", 4, val, len) == false) {
				return 0;
			}
		} else if (strcmp(data->text->ptr, "duration_in_seconds") == 0) {
			if (serialize_array(&data->feed.item->attachments, "duration", 8, val, len) == false) {
				return 0;
			}
		}
	}
	return 1;
}

static int
string_handler(void *ctx, const unsigned char *val, size_t len)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon string.");
	if (data->path[data->depth] == JSON_OBJECT_ITEM) {
		return item_string_handler(data, (char *)val, len);
	} else if (we_are_inside_item(data) == true) {
		if (data->path[data->depth] == JSON_OBJECT_AUTHOR) {
			return person_string_handler(&data->feed.item->persons, data->text, (char *)val, len);
		} else if (data->path[data->depth] == JSON_OBJECT_ATTACHMENT) {
			return attachment_string_handler(&data->feed.item->attachments, data->text, (char *)val, len);
		} else if (data->path[data->depth] == JSON_ARRAY_TAGS) {
			if (serialize_caret(&data->feed.item->extras) == false) {
				return 0;
			}
			if (serialize_array(&data->feed.item->extras, "category", 8, (char *)val, len) == false) {
				return 0;
			}
		}
	} else if (data->path[data->depth] == JSON_OBJECT_AUTHOR) {
		return person_string_handler(&data->feed.persons, data->text, (char *)val, len);
	} else if ((data->depth < 2) && (data->path[data->depth] == JSON_OBJECT_UNKNOWN)) {
		return feed_string_handler(data, (char *)val, len);
	}
	return 1;
}

static int
start_map_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon the beginning of an object.");
	data->depth += 1;
	if (we_are_inside_item(data) == true) {
		if (data->path[data->depth - 1] == JSON_ARRAY_AUTHORS) {
			data->path[data->depth] = JSON_OBJECT_AUTHOR;
			if (serialize_caret(&data->feed.item->persons) == false) {
				return 0;
			}
			if (serialize_array(&data->feed.item->persons, "type", 4, "author", 6) == false) {
				return 0;
			}
		} else if (data->path[data->depth - 1] == JSON_ARRAY_ATTACHMENTS) {
			data->path[data->depth] = JSON_OBJECT_ATTACHMENT;
			if (serialize_caret(&data->feed.item->attachments) == false) {
				return 0;
			}
		} else {
			data->path[data->depth] = JSON_OBJECT_UNKNOWN;
		}
	} else if (data->path[data->depth - 1] == JSON_ARRAY_ITEMS) {
		data->path[data->depth] = JSON_OBJECT_ITEM;
		if (prepend_item(&data->feed.item) == false) {
			return 0;
		}
	} else if (data->path[data->depth - 1] == JSON_ARRAY_AUTHORS) {
		data->path[data->depth] = JSON_OBJECT_AUTHOR;
		if (serialize_caret(&data->feed.persons) == false) {
			return 0;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "author", 6) == false) {
			return 0;
		}
	} else {
		data->path[data->depth] = JSON_OBJECT_UNKNOWN;
	}
	return 1;
}

static int
map_key_handler(void *ctx, const unsigned char *key, size_t key_len)
{
	struct stream_callback_data *data = ctx;
	bool success = cpyas(data->text, (char *)key, key_len);
	INFO("Stumbled upon key: %s", data->text->ptr);
	return success;
}

static int
start_array_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon the beginning of an array.");
	data->depth += 1;
	if (ISARRAY(data->path[data->depth - 1])) {
		// JSON Feed doesn't have nested arrays!
		data->path[data->depth] = JSON_ARRAY_UNKNOWN;
	} else if (strcmp(data->text->ptr, "items") == 0) {
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
	return 1;
}

static int
end_of_object_or_array_handler(void *ctx)
{
	struct stream_callback_data *data = ctx;
	INFO("Stumbled upon the end of an object/array.");
	if (data->depth > 0) {
		data->depth -= 1;
	}
	return 1;
}

static const yajl_callbacks callbacks = {
	null_handler,
	boolean_handler,
	NULL, // Integer handler is ignored when number handler is set.
	NULL, // Double handler is ignored when number handler is set.
	&number_handler,
	&string_handler,
	&start_map_handler,
	&map_key_handler,
	&end_of_object_or_array_handler,
	&start_array_handler,
	&end_of_object_or_array_handler
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
	data->path[0] = JSON_OBJECT_UNKNOWN;
	return true;
}

void
free_json_parser(struct stream_callback_data *data)
{
	yajl_complete_parse(data->json_parser); // final parsing call
	yajl_free(data->json_parser);
	free_string(data->text);
}
