#include "update_feed/parse_feed/xml/parse_xml_feed.h"

static int8_t
title_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if ((data->feed.item->title.value == NULL) || (data->feed.item->title.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->title.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if ((data->feed.title.value == NULL) || (data->feed.title.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.title.value, data->text) == false) {
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
			if (crtss_or_cpyss(&data->feed.item->summary.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if ((data->feed.summary.value == NULL) || (data->feed.summary.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.summary.value, data->text) == false) {
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
		if (cat_array_to_serialization(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
contributor_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_array_to_serialization(&data->feed.item->persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
subject_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_string_to_serialization(&data->feed.item->categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (cat_string_to_serialization(&data->feed.categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_dublincore_handlers[] = {
	{"title",       DC_TITLE,        NULL, &title_end},
	{"description", DC_DESCRIPTION,  NULL, &description_end},
	{"creator",     DC_CREATOR,      NULL, &creator_end},
	{"contributor", DC_CONTRIBUTOR,  NULL, &contributor_end},
	{"subject",     DC_SUBJECT,      NULL, &subject_end},
	{NULL,          XML_UNKNOWN_POS, NULL, NULL},
};
