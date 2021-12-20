#include <string.h>
#include <stdbool.h>
#include "update_feed/update_feed.h"

bool
we_are_inside_item(const struct parser_data *data)
{
	if (
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
		((data->rss20_pos  & RSS20_ITEM)   != 0) ||
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
		((data->atom10_pos & ATOM10_ENTRY) != 0) ||
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
		((data->atom03_pos & ATOM03_ENTRY) != 0) ||
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
		((data->rss11_pos  & RSS11_ITEM)   != 0) ||
#endif
		0
	) {
		return true;
	}
	return false;
}

const char *
get_value_of_attribute_key(const XML_Char **atts, const char *key)
{
	/* expat says that atts is name/value array (that is {name1, value1, name2, value2, ...})
	 * hence do iterate with the step of 2 */
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], key) == 0) {
			return atts[i + 1]; // success
		}
	}
	return NULL; // failure, didn't find an attribute with key name
}
