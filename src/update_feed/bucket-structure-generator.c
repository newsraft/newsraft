#include "update_feed/update_feed.h"

struct string *
generate_generator_string_for_database(const struct generator *generator)
{
	struct string *str = crtss(generator->name);
	if (str == NULL) {
		return NULL;
	}
	if (generator->version->len != 0) {
		if (catcs(str, ' ') == false) {
			goto error;
		}
		if (catss(str, generator->version) == false) {
			goto error;
		}
	}
	if (generator->url->len != 0) {
		if (catcs(str, ' ') == false) {
			goto error;
		}
		if (catss(str, generator->url) == false) {
			goto error;
		}
	}
	return str;
error:
	free_string(str);
	return NULL;
}
