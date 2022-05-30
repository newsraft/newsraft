#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

static int8_t
title_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if ((data->feed.item->title.value == NULL) || (data->feed.item->title.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->title.value, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if ((data->feed.title.value == NULL) || (data->feed.title.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.title.value, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.title.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if ((data->feed.item->summary.value == NULL) || (data->feed.item->summary.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->summary.value, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if ((data->feed.summary.value == NULL) || (data->feed.summary.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.summary.value, data->value) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.summary.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
creator_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (prepend_person(&data->feed.item->author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.item->author->name = crtss(data->value);
		if (data->feed.item->author->name == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (prepend_person(&data->feed.author) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.author->name = crtss(data->value);
		if (data->feed.author->name == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subject_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (prepend_category(&data->feed.item->category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.item->category->term = crtss(data->value);
		if (data->feed.item->category->term == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (prepend_category(&data->feed.category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.category->term = crtss(data->value);
		if (data->feed.category->term == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_dublincore_handlers[] = {
	{"title",       DC_TITLE,       NULL, &title_end},
	{"description", DC_DESCRIPTION, NULL, &description_end},
	{"creator",     DC_CREATOR,     NULL, &creator_end},
	{"contributor", DC_CONTRIBUTOR, NULL, &creator_end},
	{"subject",     DC_SUBJECT,     NULL, &subject_end},
	{NULL,          DC_NONE,        NULL, NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
