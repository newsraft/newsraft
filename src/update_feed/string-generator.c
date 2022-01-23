#include "update_feed/update_feed.h"

struct string *
generate_generator_string(const struct getfeed_generator *generator)
{
	struct string *str = crtss((struct string *)generator->name);
	if (str == NULL) {
		return NULL;
	}

	if (generator->version->len != 0) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
		}
		if (catss(str, (struct string *)generator->version) == false) {
			goto error;
		}
	}

	if (generator->url->len != 0) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
		}
		if (catss(str, (struct string *)generator->url) == false) {
			goto error;
		}
	}

	return str;

error:
	free_string(str);
	return NULL;
}
