#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

static inline void
title_start(struct xml_data *data)
{
	data->dc_pos |= DC_TITLE;
}

static inline void
title_end(struct xml_data *data)
{
	if ((data->dc_pos & DC_TITLE) == 0) {
		return;
	}
	data->dc_pos &= ~DC_TITLE;
	if (we_are_inside_item(data) == true) {
		if (data->feed->item->title.value->len == 0) {
			if (cpyss(data->feed->item->title.value, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (cpyss(data->feed->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
creator_start(struct xml_data *data)
{
	data->dc_pos |= DC_CREATOR;
}

static inline void
creator_end(struct xml_data *data)
{
	if ((data->dc_pos & DC_CREATOR) == 0) {
		return;
	}
	data->dc_pos &= ~DC_CREATOR;
	if (we_are_inside_item(data) == true) {
		if (prepend_person(&data->feed->item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyss(data->feed->item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (prepend_person(&data->feed->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (cpyss(data->feed->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
description_start(struct xml_data *data)
{
	data->dc_pos |= DC_DESCRIPTION;
}

static inline void
description_end(struct xml_data *data)
{
	if ((data->dc_pos & DC_DESCRIPTION) == 0) {
		return;
	}
	data->dc_pos &= ~DC_DESCRIPTION;
	if (we_are_inside_item(data) == true) {
		if (data->feed->item->summary.value->len == 0) {
			if (cpyss(data->feed->item->summary.value, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (cpyss(data->feed->summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static inline void
subject_start(struct xml_data *data)
{
	data->dc_pos |= DC_SUBJECT;
}

static inline void
subject_end(struct xml_data *data)
{
	if ((data->dc_pos & DC_SUBJECT) == 0) {
		return;
	}
	data->dc_pos &= ~DC_SUBJECT;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (prepend_category(&data->feed->item->category) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyss(data->feed->item->category->term, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_dc_element_start(struct xml_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "title")   == 0)     { title_start(data);       }
	else if (strcmp(name, "creator") == 0)     { creator_start(data);     }
	else if (strcmp(name, "contributor") == 0) { creator_start(data);     }
	else if (strcmp(name, "description") == 0) { description_start(data); }
	else if (strcmp(name, "subject") == 0)     { subject_start(data);     }
}

void
parse_dc_element_end(struct xml_data *data, const XML_Char *name)
{
	     if (strcmp(name, "title") == 0)       { title_end(data);       }
	else if (strcmp(name, "creator") == 0)     { creator_end(data);     }
	else if (strcmp(name, "contributor") == 0) { creator_end(data);     }
	else if (strcmp(name, "description") == 0) { description_end(data); }
	else if (strcmp(name, "subject") == 0)     { subject_end(data);     }
}
#endif // FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
