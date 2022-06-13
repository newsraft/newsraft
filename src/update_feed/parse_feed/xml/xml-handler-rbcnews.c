#include "update_feed/parse_feed/xml/parse_xml_feed.h"

static int8_t
full_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		// Save content only if it's longer than the content we currently have.
		if ((data->feed.item->content.value == NULL) || (data->text->len > data->feed.item->content.value->len)) {
			if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (crtas_or_cpyas(&data->feed.item->content.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
tag_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_string_to_serialization(&data->feed.item->categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rbcnews_handlers[] = {
	{"full-text", XML_UNKNOWN_POS, NULL, &full_text_end},
	{"tag",       XML_UNKNOWN_POS, NULL, &tag_end},
	{NULL,        XML_UNKNOWN_POS, NULL, NULL},
};
