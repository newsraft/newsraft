#include <string.h>
#include "update_feed/parse_feed/xml/parse_xml_feed.h"

bool
we_are_inside_item(const struct stream_callback_data *data)
{
	for (size_t i = 0; i <= data->depth; ++i) {
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
		if (data->path[i] == ATOM10_ENTRY) {
			return true;
		}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
		if (data->path[i] == RSS20_ITEM) {
			return true;
		}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
		if (data->path[i] == RSS11_ITEM) {
			return true;
		}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
		if (data->path[i] == ATOM03_ENTRY) {
			return true;
		}
#endif
	}
	return false;
}

const char *
get_value_of_attribute_key(const XML_Char **attrs, const char *key)
{
	for (size_t i = 0; attrs[i] != NULL; i += 2) {
		if (strcmp(key, attrs[i]) == 0) {
			return attrs[i + 1];
		}
	}
	return NULL;
}

bool
serialize_attribute(struct string **dest, const XML_Char **attrs, const char *attr_key, const char *prefix, size_t prefix_len)
{
	const char *attr_value = get_value_of_attribute_key(attrs, attr_key);
	if (attr_value == NULL) {
		return true; // Ignore absence of an attribute.
	}
	const size_t attr_value_len = strlen(attr_value);
	if (attr_value_len == 0) {
		return true; // Ignore empty attribute values.
	}
	if (cat_array_to_serialization(dest, prefix, prefix_len, attr_value, attr_value_len) == false) {
		return false;
	}
	return true;
}

int8_t
generator_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)data;
	const char *attr = get_value_of_attribute_key(attrs, "uri");
	if (attr != NULL) {
		INFO("Feed generator link (URI): %s", attr);
	}
	attr = get_value_of_attribute_key(attrs, "url");
	if (attr != NULL) {
		INFO("Feed generator link (URL): %s", attr);
	}
	attr = get_value_of_attribute_key(attrs, "version");
	if (attr != NULL) {
		INFO("Feed generator version: %s", attr);
	}
	return PARSE_OKAY;
}

int8_t
generator_end(struct stream_callback_data *data)
{
	INFO("Feed generator name: %s", data->text->ptr);
	return PARSE_OKAY;
}
