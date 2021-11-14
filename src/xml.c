#include <string.h>
#include "feedeater.h"

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
