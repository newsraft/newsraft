#include <string.h>
#include "parse_xml/parse_xml_feed.h"

// https://web.archive.org/web/20211118181732/https://validator.w3.org/feed/docs/atom.html
// https://web.archive.org/web/20211201194224/https://datatracker.ietf.org/doc/html/rfc4287

static int8_t
atom_link_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "href");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty links.
	}
	const char *rel = get_value_of_attribute_key(attrs, "rel");
	if (rel != NULL && strcmp(rel, "self") == 0) {
		return PARSE_OKAY; // Ignore links to feed itself.
	}
	// Default value of rel is alternate!
	if (rel == NULL || strcmp(rel, "alternate") == 0) {
		if (data->path[data->depth] == GENERIC_ITEM) {
			cpyas(&data->feed.item->url, attr, attr_len);
		} else if (data->path[data->depth] == GENERIC_FEED) {
			cpyas(&data->feed.url, attr, attr_len);
		}
	} else if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->attachments);
		serialize_array(&data->feed.item->attachments, "url=", 4, attr, attr_len);
		serialize_attribute(&data->feed.item->attachments, "type=", 5, attrs, "type");
		serialize_attribute(&data->feed.item->attachments, "size=", 5, attrs, "length");
	}
	return PARSE_OKAY;
}

static int8_t
atom_content_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->content);
		serialize_attribute(&data->feed.item->content, "type=", 5, attrs, "type");
		const char *type = get_value_of_attribute_key(attrs, "type");
		if (type != NULL && strstr(type, "xhtml") != NULL) {
			data->emptying_target = &data->decoy;
			empty_string(data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
atom_content_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->emptying_target = data->text;
		serialize_string(&data->feed.item->content, "text=", 5, data->text);
	}
	return PARSE_OKAY;
}

static int8_t
published_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->publication_date = parse_date(data->text->ptr, true);
	}
	return PARSE_OKAY;
}

static int8_t
author_start(struct feed_update_state *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->persons);
		serialize_array(&data->feed.item->persons, "type=", 5, "author", 6);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "author", 6);
	}
	return PARSE_OKAY;
}

static int8_t
contributor_start(struct feed_update_state *data, const XML_Char **attrs)
{
	(void)attrs;
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->persons);
		serialize_array(&data->feed.item->persons, "type=", 5, "contributor", 11);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "contributor", 11);
	}
	return PARSE_OKAY;
}

static int8_t
name_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == ATOM_AUTHOR) {
		if (data->in_item == true) {
			serialize_string(&data->feed.item->persons, "name=", 5, data->text);
		} else {
			serialize_string(&data->feed.persons, "name=", 5, data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
uri_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == ATOM_AUTHOR) {
		if (data->in_item == true) {
			serialize_string(&data->feed.item->persons, "url=", 4, data->text);
		} else {
			serialize_string(&data->feed.persons, "url=", 4, data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
email_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == ATOM_AUTHOR) {
		if (data->in_item == true) {
			serialize_string(&data->feed.item->persons, "email=", 6, data->text);
		} else {
			serialize_string(&data->feed.persons, "email=", 6, data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
atom_category_start(struct feed_update_state *data, const XML_Char **attrs)
{
	struct string **target;
	if (data->path[data->depth] == GENERIC_ITEM) {
		target = &data->feed.item->extras;
	} else if (data->path[data->depth] == GENERIC_FEED) {
		target = &data->feed.extras;
	} else {
		return PARSE_OKAY; // Ignore misplaced categories.
	}
	const char *attr = get_value_of_attribute_key(attrs, "label");
	if (attr == NULL) {
		attr = get_value_of_attribute_key(attrs, "term");
		if (attr == NULL) {
			return PARSE_OKAY; // Ignore empty categories.
		}
	}
	const size_t attr_len = strlen(attr);
	if (attr_len != 0) {
		serialize_caret(target);
		serialize_array(target, "category=", 9, attr, attr_len);
	}
	return PARSE_OKAY;
}

static int8_t
atom_subtitle_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		serialize_caret(&data->feed.content);
		serialize_attribute(&data->feed.content, "type=", 5, attrs, "type");
	}
	return PARSE_OKAY;
}

static int8_t
atom_subtitle_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		serialize_string(&data->feed.content, "text=", 5, data->text);
	}
	return PARSE_OKAY;
}

static int8_t
atom_generator_start(struct feed_update_state *data, const XML_Char **attrs)
{
	(void)data;
	const char *attr = get_value_of_attribute_key(attrs, "uri");
	if (attr != NULL) {
		INFO("Feed generator URI: %s", attr);
	}
	attr = get_value_of_attribute_key(attrs, "url");
	if (attr != NULL) {
		INFO("Feed generator URL: %s", attr);
	}
	attr = get_value_of_attribute_key(attrs, "version");
	if (attr != NULL) {
		INFO("Feed generator version: %s", attr);
	}
	return PARSE_OKAY;
}
