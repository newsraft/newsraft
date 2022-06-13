#include <stdio.h>
#include "update_feed/update_feed.h"

void
empty_link(struct getfeed_temp *temp)
{
	empty_string_safe(temp->attachment.url);
	empty_string_safe(temp->attachment.type);
	temp->attachment.size = 0;
	temp->attachment.duration = 0;
}

bool
serialize_link(struct getfeed_temp *temp, struct string **str)
{
	if ((temp->attachment.url == NULL) || (temp->attachment.url->len == 0)) {
		return true;
	}
	if (cat_string_to_serialization(str, "url", 3, temp->attachment.url) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "type", 4, temp->attachment.type) == false) {
		return false;
	}
	if (temp->attachment.size != 0) {
		temp->buf_len = snprintf(temp->buf, 100, "%zu", temp->attachment.size);
		if ((temp->buf_len > 0) && (temp->buf_len < 100)) {
			if (cat_array_to_serialization(str, "size", 4, temp->buf, temp->buf_len) == false) {
				return false;
			}
		}
	}
	if (temp->attachment.duration != 0) {
		temp->buf_len = snprintf(temp->buf, 100, "%zu", temp->attachment.duration);
		if ((temp->buf_len > 0) && (temp->buf_len < 100)) {
			if (cat_array_to_serialization(str, "duration", 8, temp->buf, temp->buf_len) == false) {
				return false;
			}
		}
	}
	return true;
}
