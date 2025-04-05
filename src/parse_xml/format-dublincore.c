#include "parse_xml/parse_xml_feed.h"

static int8_t
dublincore_title_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		if (STRING_IS_EMPTY(data->feed.item->title)) {
			cpyss(&data->feed.item->title, data->text);
		}
	} else {
		if (STRING_IS_EMPTY(data->feed.title)) {
			cpyss(&data->feed.title, data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
dublincore_creator_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		serialize_caret(&data->feed.item->persons);
		serialize_array(&data->feed.item->persons, "type=", 5, "author", 6);
		serialize_string(&data->feed.item->persons, "name=", 5, data->text);
	} else {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "author", 6);
		serialize_string(&data->feed.persons, "name=", 5, data->text);
	}
	return PARSE_OKAY;
}

static int8_t
dublincore_contributor_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		serialize_caret(&data->feed.item->persons);
		serialize_array(&data->feed.item->persons, "type=", 5, "contributor", 11);
		serialize_string(&data->feed.item->persons, "name=", 5, data->text);
	} else {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "contributor", 11);
		serialize_string(&data->feed.persons, "name=", 5, data->text);
	}
	return PARSE_OKAY;
}
