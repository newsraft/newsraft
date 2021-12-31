#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static inline void
title_start(struct parser_data *data)
{
	data->dc_pos |= DC_TITLE;
}

static inline void
title_end(struct parser_data *data)
{
	if ((data->dc_pos & DC_TITLE) == 0) {
		return;
	}
	data->dc_pos &= ~DC_TITLE;
	if (we_are_inside_item(data) == true) {
		if (data->bucket->title->len == 0) {
			if (cpyss(data->bucket->title, data->value) != 0) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (cpyss(data->feed->title, data->value) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
creator_start(struct parser_data *data)
{
	data->dc_pos |= DC_CREATOR;
}

static inline void
creator_end(struct parser_data *data)
{
	if ((data->dc_pos & DC_CREATOR) == 0) {
		return;
	}
	data->dc_pos &= ~DC_CREATOR;
	if (we_are_inside_item(data) == true) {
		if (expand_authors_of_item_bucket_by_one_element(data->bucket) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (add_name_to_last_author_of_item_bucket(data->bucket, data->value) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		// TODO update feed author...
	}
}

static inline void
description_start(struct parser_data *data)
{
	data->dc_pos |= DC_DESCRIPTION;
}

static inline void
description_end(struct parser_data *data)
{
	if ((data->dc_pos & DC_DESCRIPTION) == 0) {
		return;
	}
	data->dc_pos &= ~DC_DESCRIPTION;
	if (we_are_inside_item(data) == true) {
		if (data->bucket->summary->len == 0) {
			if (cpyss(data->bucket->summary, data->value) != 0) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (cpyss(data->feed->summary, data->value) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
subject_start(struct parser_data *data)
{
	data->dc_pos |= DC_SUBJECT;
}

static inline void
subject_end(struct parser_data *data)
{
	if ((data->dc_pos & DC_SUBJECT) == 0) {
		return;
	}
	data->dc_pos &= ~DC_SUBJECT;
	if (we_are_inside_item(data) == true) {
		if (add_category_to_item_bucket(data->bucket, data->value->ptr, data->value->len) != 0) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

void
parse_dc_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "title")   == 0)     { title_start(data);       }
	else if (strcmp(name, "creator") == 0)     { creator_start(data);     }
	else if (strcmp(name, "contributor") == 0) { creator_start(data);     }
	else if (strcmp(name, "description") == 0) { description_start(data); }
	else if (strcmp(name, "subject") == 0)     { subject_start(data);     }
}

void
parse_dc_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "title") == 0)       { title_end(data);       }
	else if (strcmp(name, "creator") == 0)     { creator_end(data);     }
	else if (strcmp(name, "contributor") == 0) { creator_end(data);     }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "subject") == 0)     { subject_end(data);     }
}
#endif // FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
