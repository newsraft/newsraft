#include "update_feed/update_feed.h"

struct string *
generate_generator_string(const struct getfeed_generator *generator)
{
	struct string *str;
	if (generator->name != NULL) {
		str = crtss(generator->name);
	} else {
		str = crtes();
	}
	if (str == NULL) {
		return NULL;
	}

	if ((generator->version != NULL) && (generator->version->len != 0)) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
		}
		if (catss(str, generator->version) == false) {
			goto error;
		}
	}

	if ((generator->url != NULL) && (generator->url->len != 0)) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
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
