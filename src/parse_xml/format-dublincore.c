#include "parse_xml/parse_xml_feed.h"

static void
dublincore_title_end(struct feed_update_state *data)
{
	if (data->in_item) {
		if (STRING_IS_EMPTY(data->feed.item->title)) {
			cpyss(&data->feed.item->title, data->text);
		}
	} else {
		if (STRING_IS_EMPTY(data->feed.title)) {
			cpyss(&data->feed.title, data->text);
		}
	}
}

static void
dublincore_creator_end(struct feed_update_state *data)
{
	struct string **dest = data->in_item ? &data->feed.item->persons : &data->feed.persons;
	serialize_caret(dest);
	serialize_array(dest, "type=", 5, "author", 6);
	serialize_string(dest, "name=", 5, data->text);
}

static void
dublincore_contributor_end(struct feed_update_state *data)
{
	struct string **dest = data->in_item ? &data->feed.item->persons : &data->feed.persons;
	serialize_caret(dest);
	serialize_array(dest, "type=", 5, "contributor", 11);
	serialize_string(dest, "name=", 5, data->text);
}
