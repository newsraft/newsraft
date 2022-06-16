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
copy_type_of_text_construct(struct string **dest, const XML_Char **atts)
{
	const char *type = get_value_of_attribute_key(atts, "type");
	if (type != NULL) {
		if ((strcmp(type, "html") == 0) || (strcmp(type, "xhtml") == 0)) {
			if (crtas_or_cpyas(dest, "text/html", 9) == false) {
				return false;
			}
			return true;
		}
	}
	if (crtas_or_cpyas(dest, "text/plain", 10) == false) {
		return false;
	}
	return true;
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
