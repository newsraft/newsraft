#include <stdio.h>
#include "update_feed/update_feed.h"

void
empty_picture(struct getfeed_temp *temp)
{
	empty_string_safe(temp->picture.url);
	empty_string_safe(temp->picture.type);
	temp->picture.width = 0;
	temp->picture.height = 0;
	temp->picture.size = 0;
}

bool
serialize_picture(struct getfeed_temp *temp, struct string **str)
{
	if ((temp->picture.url == NULL) || (temp->picture.url->len == 0)) {
		return true;
	}
	if (cat_string_to_serialization(str, "url", 3, temp->picture.url) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "type", 4, temp->picture.type) == false) {
		return false;
	}
	if (temp->picture.width != 0) {
		temp->buf_len = snprintf(temp->buf, 100, "%zu", temp->picture.width);
		if ((temp->buf_len > 0) && (temp->buf_len < 100)) {
			if (cat_array_to_serialization(str, "width", 5, temp->buf, temp->buf_len) == false) {
				return false;
			}
		}
	}
	if (temp->picture.height != 0) {
		temp->buf_len = snprintf(temp->buf, 100, "%zu", temp->picture.height);
		if ((temp->buf_len > 0) && (temp->buf_len < 100)) {
			if (cat_array_to_serialization(str, "height", 6, temp->buf, temp->buf_len) == false) {
				return false;
			}
		}
	}
	if (temp->picture.size != 0) {
		temp->buf_len = snprintf(temp->buf, 100, "%zu", temp->picture.size);
		if ((temp->buf_len > 0) && (temp->buf_len < 100)) {
			if (cat_array_to_serialization(str, "size", 4, temp->buf, temp->buf_len) == false) {
				return false;
			}
		}
	}
	return true;
}
