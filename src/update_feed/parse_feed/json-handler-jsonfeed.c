#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

static inline void
jsonfeed_process_string(const char *name, const char *value, size_t value_len, struct json_data *data)
{
	if (name == NULL) {
		return;
	}
	if ((data->jsonfeed_pos & JSONFEED_ITEMS) != 0) {
		if ((data->jsonfeed_pos & JSONFEED_AUTHORS) != 0) {
			if (strcmp(name, "name") == 0) {
				cpyas(data->feed->item->author->name, value, value_len);
			} else if (strcmp(name, "url") == 0) {
				cpyas(data->feed->item->author->url, value, value_len);
			} else if (strcmp(name, "avatar") == 0) {
				// picture that represents a person...
				// todo
			}
		} else if ((data->jsonfeed_pos & JSONFEED_ATTACHMENTS) != 0) {
			if (strcmp(name, "url") == 0) {
				cpyas(data->feed->item->attachment->url, value, value_len);
			} else if (strcmp(name, "mime_type") == 0) {
				cpyas(data->feed->item->attachment->type, value, value_len);
			} else if (strcmp(name, "size_in_bytes") == 0) {
				data->feed->item->attachment->size = convert_string_to_size_t_or_zero(value);
			} else if (strcmp(name, "duration_in_seconds") == 0) {
				data->feed->item->attachment->duration = convert_string_to_size_t_or_zero(value);
			}
		} else if ((data->jsonfeed_pos & JSONFEED_TAGS) != 0) {
			prepend_category(&data->feed->item->category);
			cpyas(data->feed->item->category->term, value, value_len);
		} else if (strcmp(name, "id") == 0) {
			cpyas(data->feed->item->guid, value, value_len);
		} else if (strcmp(name, "url") == 0) {
			cpyas(data->feed->item->url, value, value_len);
		} else if (strcmp(name, "title") == 0) {
			cpyas(data->feed->item->title.value, value, value_len);
			cpyas(data->feed->item->title.type, "text/plain", 10);
		} else if (strcmp(name, "content_html") == 0) {
			cpyas(data->feed->item->content.value, value, value_len);
			cpyas(data->feed->item->content.type, "text/html", 9);
		} else if (strcmp(name, "content_text") == 0) {
			cpyas(data->feed->item->content.value, value, value_len);
			cpyas(data->feed->item->content.type, "text/plain", 10);
		} else if (strcmp(name, "summary") == 0) {
			cpyas(data->feed->item->summary.value, value, value_len);
			cpyas(data->feed->item->summary.type, "text/plain", 10);
		} else if (strcmp(name, "date_published") == 0) {
			data->feed->item->pubdate = parse_date_rfc3339(value, value_len);
		} else if (strcmp(name, "date_modified") == 0) {
			data->feed->item->upddate = parse_date_rfc3339(value, value_len);
		}
	} else {
		if ((data->jsonfeed_pos & JSONFEED_AUTHORS) != 0) {
			if (strcmp(name, "name") == 0) {
				cpyas(data->feed->author->name, value, value_len);
			} else if (strcmp(name, "url") == 0) {
				cpyas(data->feed->author->url, value, value_len);
			} else if (strcmp(name, "avatar") == 0) {
				// picture that represents a person...
				// todo
			}
		} else if (strcmp(name, "title") == 0) {
			cpyas(data->feed->title.value, value, value_len);
			cpyas(data->feed->title.type, "text/plain", 10);
		} else if (strcmp(name, "home_page_url") == 0) {
			cpyas(data->feed->url, value, value_len);
		} else if (strcmp(name, "description") == 0) {
			cpyas(data->feed->summary.value, value, value_len);
			cpyas(data->feed->summary.type, "text/plain", 10);
		} else if (strcmp(name, "language") == 0) {
			cpyas(data->feed->language, value, value_len);
		}
	}
}

void
json_dump_jsonfeed(cJSON *json, struct json_data *data)
{
	if (json == NULL) {
		return;
	}
	for (cJSON *node = json; node != NULL; node = node->next) {
		if (cJSON_IsString(node) == true) {
			if (node->valuestring == NULL) {
				jsonfeed_process_string(node->string, "", 0, data);
			} else {
				jsonfeed_process_string(node->string, node->valuestring, strlen(node->valuestring), data);
			}
		} else if (cJSON_IsObject(node) == true) {
			if (data->jsonfeed_pos == JSONFEED_ITEMS) {
				prepend_item(&data->feed->item);
			} else if ((data->jsonfeed_pos & JSONFEED_AUTHORS) != 0) {
				if ((data->jsonfeed_pos & JSONFEED_ITEMS) != 0) {
					prepend_person(&data->feed->item->author);
				} else {
					prepend_person(&data->feed->author);
				}
			} else if ((data->jsonfeed_pos & JSONFEED_ATTACHMENTS) != 0) {
				if ((data->jsonfeed_pos & JSONFEED_ITEMS) != 0) {
					prepend_link(&data->feed->item->attachment);
				} else {
					// Spec says feed can't have attachments.
				}
			}
			json_dump_jsonfeed(node->child, data);
		} else if (cJSON_IsArray(node) == true) {
			int8_t new_pos = JSONFEED_NONE;
			if (strcmp(node->string, "authors") == 0) {
				new_pos = JSONFEED_AUTHORS;
			} else if (strcmp(node->string, "tags") == 0) {
				new_pos = JSONFEED_TAGS;
			} else if (strcmp(node->string, "attachments") == 0) {
				new_pos = JSONFEED_ATTACHMENTS;
			} else if (strcmp(node->string, "items") == 0) {
				new_pos = JSONFEED_ITEMS;
			}
			data->jsonfeed_pos |= new_pos;
			json_dump_jsonfeed(node->child, data);
			data->jsonfeed_pos &= ~new_pos;
		} else if (cJSON_IsBool(node) == true) {
			// expired boolean
		}
	}
}
#endif // FEEDEATER_FORMAT_SUPPORT_JSONFEED
