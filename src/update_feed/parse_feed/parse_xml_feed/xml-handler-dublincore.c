#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

static void
title_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == true) {
		if ((data->feed->item->title.value == NULL) || (data->feed->item->title.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed->item->title.value, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (crtss_or_cpyss(&data->feed->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
description_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == true) {
		if ((data->feed->item->summary.value == NULL) || (data->feed->item->summary.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed->item->summary.value, data->value) == false) {
				data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
				return;
			}
		}
	} else {
		if (crtss_or_cpyss(&data->feed->summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
creator_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == true) {
		if (prepend_person(&data->feed->item->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtss_or_cpyss(&data->feed->item->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (prepend_person(&data->feed->author) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (crtss_or_cpyss(&data->feed->author->name, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
subject_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (prepend_category(&data->feed->item->category) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (crtss_or_cpyss(&data->feed->item->category->term, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
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
