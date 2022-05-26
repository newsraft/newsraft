#include <stdlib.h>
#include "update_feed/update_feed.h"

bool
prepend_empty_picture(struct getfeed_picture **head_picture_ptr)
{
	struct getfeed_picture *picture = calloc(1, sizeof(struct getfeed_picture));
	if (picture == NULL) {
		return false;
	}
	picture->next = *head_picture_ptr;
	*head_picture_ptr = picture;
	return true;
}

void
reverse_picture_list(struct getfeed_picture **head_picture_ptr)
{
	struct getfeed_picture *prev = NULL;
	struct getfeed_picture *current = *head_picture_ptr;
	struct getfeed_picture *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*head_picture_ptr = prev;
}

void
free_picture(struct getfeed_picture *picture)
{
	struct getfeed_picture *temp;
	struct getfeed_picture *p = picture;
	while (p != NULL) {
		free_string(p->url);
		free_string(p->type);
		temp = p;
		p = p->next;
		free(temp);
	}
}

struct string *
generate_picture_list_string(const struct getfeed_picture *picture)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	char tmp[100];
	int64_t tmp_len;
	const struct getfeed_picture *p = picture;
	while (p != NULL) {
		if ((p->url == NULL) || (p->url->len == 0)) {
			continue;
		}
		if (str->len != 0) {
			if (catcs(str, '\n') == false) { goto error; }
		}
		if (catss(str, p->url) == false) { goto error; }
		if (catcs(str, ' ') == false) { goto error; }
		if (p->type != NULL) {
			if (catss(str, p->type) == false) { goto error; }
		}
		// Mind the heading space character! It's needed for separation.
		tmp_len = snprintf(tmp, 100, " %zu", p->width);
		if ((tmp_len < 0) || (tmp_len >= 100)) { goto error; }
		if (catas(str, tmp, tmp_len) == false) { goto error; }
		tmp_len = snprintf(tmp, 100, " %zu", p->height);
		if ((tmp_len < 0) || (tmp_len >= 100)) { goto error; }
		if (catas(str, tmp, tmp_len) == false) { goto error; }
		tmp_len = snprintf(tmp, 100, " %zu", p->size);
		if ((tmp_len < 0) || (tmp_len >= 100)) { goto error; }
		if (catas(str, tmp, tmp_len) == false) { goto error; }
		p = p->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
