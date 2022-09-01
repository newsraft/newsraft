#include "update_feed/parse_xml/parse_xml_feed.h"

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if ((data->feed.item->title == NULL) || (data->feed.item->title->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->title, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		if ((data->feed.title == NULL) || (data->feed.title->len == 0)) {
			if (crtss_or_cpyss(&data->feed.title, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
creator_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
contributor_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "contributor", 11) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "name", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_dublincore_handlers[] = {
	{"title",       DC_TITLE,        NULL, &title_end},
	{"description", XML_UNKNOWN_POS, NULL, &generic_plain_content_end},
	{"creator",     DC_CREATOR,      NULL, &creator_end},
	{"contributor", DC_CONTRIBUTOR,  NULL, &contributor_end},
	{"subject",     XML_UNKNOWN_POS, NULL, &generic_category_end},
	{NULL,          XML_UNKNOWN_POS, NULL, NULL},
};
