#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

bool
we_are_inside_item(const struct stream_callback_data *data)
{
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
	if ((data->xml_pos[ATOM10_FORMAT] & ATOM10_ENTRY) != 0) {
		return true;
	}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
	if ((data->xml_pos[RSS20_FORMAT] & RSS20_ITEM) != 0) {
		return true;
	}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
	if ((data->xml_pos[RSS11_FORMAT] & RSS11_ITEM) != 0) {
		return true;
	}
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
	if ((data->xml_pos[ATOM03_FORMAT] & ATOM03_ENTRY) != 0) {
		return true;
	}
#endif
	return false;
}

const char *
get_value_of_attribute_key(const XML_Char **atts, const char *key)
{
	for (size_t i = 0; atts[i] != NULL; i += 2) {
		if (strcmp(key, atts[i]) == 0) {
			return atts[i + 1];
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
