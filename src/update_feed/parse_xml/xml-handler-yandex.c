#include "update_feed/parse_xml/parse_xml_feed.h"

// https://yandex.ru/support/news/feed.html

static int8_t
bind_to_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->attachments, "url", 3, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->attachments, "content", 7, "source", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_yandex_handlers[] = {
	// <official-comment> is a container for most of this stuff but it is quite redundant.
	{"full-text",    XML_UNKNOWN_POS, NULL, &generic_html_content_end},
	{"comment-text", XML_UNKNOWN_POS, NULL, &generic_plain_content_end},
	{"genre",        XML_UNKNOWN_POS, NULL, &generic_category_end},
	{"bind-to",      XML_UNKNOWN_POS, NULL, &bind_to_end},
	{NULL,           XML_UNKNOWN_POS, NULL, NULL},
};
