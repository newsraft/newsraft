#ifdef NEWSRAFT_FORMAT_SUPPORT_JSONFEED
#include <string.h>
#include "update_feed/parse_feed/parse_json_feed/parse_json_feed.h"

static void
populate_person(cJSON *json, struct getfeed_person *person)
{
	for (cJSON *credit = json->child; credit != NULL; credit = credit->next) {
		if (cJSON_IsString(credit) == false) {
			// All members of the author object are strings, skip everything else.
			continue;
		}
		if ((credit->string == NULL) || (credit->valuestring == NULL)) {
			// Also skip invalid entries.
			continue;
		}
		if (strcmp(credit->string, "name") == 0) {
			crtas_or_cpyas(&person->name, credit->valuestring, strlen(credit->valuestring));
		} else if (strcmp(credit->string, "url") == 0) {
			crtas_or_cpyas(&person->url, credit->valuestring, strlen(credit->valuestring));
		}
		// JSON Feed also provides "avatar" field! Cool, nice, great... What about "email", huh?
	}
}

static void
populate_link(cJSON *json, struct getfeed_link *link)
{
	for (cJSON *entry = json->child; entry != NULL; entry = entry->next) {
		if (entry->string == NULL) {
			continue;
		}
		if (cJSON_IsString(entry) == true) {
			if (entry->valuestring == NULL) {
				continue;
			}
			if (strcmp(entry->string, "url") == 0) {
				crtas_or_cpyas(&link->url, entry->valuestring, strlen(entry->valuestring));
			} else if (strcmp(entry->string, "mime_type") == 0) {
				crtas_or_cpyas(&link->type, entry->valuestring, strlen(entry->valuestring));
			} else if (strcmp(entry->string, "title") == 0) {
				// TODO
			}
		} else if (cJSON_IsNumber(entry) == true) {
			if (strcmp(entry->string, "size_in_bytes") == 0) {
				link->size = entry->valueint;
			} else if (strcmp(entry->string, "duration_in_seconds") == 0) {
				link->duration = entry->valueint;
			}
		}
	}
}

static inline void
process_item_entry(cJSON *json, struct json_data *data)
{
	for (cJSON *node = json->child; node != NULL; node = node->next) {
		if (node->string == NULL) {
			continue;
		}
		if (cJSON_IsString(node) == true) {
			if (node->valuestring == NULL) {
				continue;
			}
			if (strcmp(node->string, "id") == 0) {
				crtas_or_cpyas(&data->feed->item->guid, node->valuestring, strlen(node->valuestring));
			} else if (strcmp(node->string, "url") == 0) {
				crtas_or_cpyas(&data->feed->item->url, node->valuestring, strlen(node->valuestring));
			} else if (strcmp(node->string, "title") == 0) {
				crtas_or_cpyas(&data->feed->item->title.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->item->title.type, "text/plain", 10);
			} else if (strcmp(node->string, "content_html") == 0) {
				crtas_or_cpyas(&data->feed->item->content.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->item->content.type, "text/html", 9);
			} else if (strcmp(node->string, "content_text") == 0) {
				crtas_or_cpyas(&data->feed->item->content.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->item->content.type, "text/plain", 10);
			} else if (strcmp(node->string, "summary") == 0) {
				crtas_or_cpyas(&data->feed->item->summary.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->item->summary.type, "text/plain", 10);
			} else if (strcmp(node->string, "date_published") == 0) {
				data->feed->item->pubdate = parse_date_rfc3339(node->valuestring, strlen(node->valuestring));
			} else if (strcmp(node->string, "date_modified") == 0) {
				data->feed->item->upddate = parse_date_rfc3339(node->valuestring, strlen(node->valuestring));
			} else if (strcmp(node->string, "external_url") == 0) {
				prepend_link(&data->feed->item->attachment);
				crtas_or_cpyas(&data->feed->item->attachment->url, node->valuestring, strlen(node->valuestring));
			}
		} else if (cJSON_IsArray(node) == true) {
			if (strcmp(node->string, "authors") == 0) {
				for (cJSON *author = node->child; author != NULL; author = author->next) {
					prepend_person(&data->feed->item->author);
					populate_person(author, data->feed->item->author);
				}
			} else if (strcmp(node->string, "attachments") == 0) {
				for (cJSON *attachment = node->child; attachment != NULL; attachment = attachment->next) {
					prepend_link(&data->feed->item->attachment);
					populate_link(attachment, data->feed->item->attachment);
				}
			} else if (strcmp(node->string, "tags") == 0) {
				for (cJSON *tag = node->child; tag != NULL; tag = tag->next) {
					if ((cJSON_IsString(tag) == false) || (tag->valuestring == NULL)) {
						// All elements of the tags array are strings, skip everything else. Also skip NULL strings.
						continue;
					}
					prepend_category(&data->feed->item->category);
					crtas_or_cpyas(&data->feed->item->category->term, tag->valuestring, strlen(tag->valuestring));
				}
			}
		}
	}
}

void
json_dump_jsonfeed(cJSON *json, struct json_data *data)
{
	if (json == NULL) {
		return;
	}
	for (cJSON *node = json->child; node != NULL; node = node->next) {
		if (node->string == NULL) {
			continue; // wtf?
		}
		if (cJSON_IsString(node) == true) {
			if (node->valuestring == NULL) {
				continue;
			}
			if (strcmp(node->string, "title") == 0) {
				crtas_or_cpyas(&data->feed->title.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->title.type, "text/plain", 10);
			} else if (strcmp(node->string, "home_page_url") == 0) {
				crtas_or_cpyas(&data->feed->url, node->valuestring, strlen(node->valuestring));
			} else if (strcmp(node->string, "description") == 0) {
				crtas_or_cpyas(&data->feed->summary.value, node->valuestring, strlen(node->valuestring));
				crtas_or_cpyas(&data->feed->summary.type, "text/plain", 10);
			} else if (strcmp(node->string, "language") == 0) {
				crtas_or_cpyas(&data->feed->language, node->valuestring, strlen(node->valuestring));
			}
		} else if (cJSON_IsArray(node) == true) {
			if (strcmp(node->string, "authors") == 0) {
				for (cJSON *author = node->child; author != NULL; author = author->next) {
					prepend_person(&data->feed->author);
					populate_person(author, data->feed->author);
				}
			} else if (strcmp(node->string, "items") == 0) {
				for (cJSON *item = node->child; item != NULL; item = item->next) {
					prepend_item(&data->feed->item);
					process_item_entry(item, data);
				}
			}
		} else if (cJSON_IsBool(node) == true) {
			// TODO expired
		}
	}
}
#endif // NEWSRAFT_FORMAT_SUPPORT_JSONFEED
