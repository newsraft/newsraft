#include "update_feed/update_feed.h"

struct string *
generate_generator_string(const struct getfeed_generator *generator)
{
	struct string *str = crtas(generator->name->ptr, generator->name->len);
	if (str == NULL) {
		return NULL;
	}

	if (generator->version->len != 0) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
		}
		if (catas(str, generator->version->ptr, generator->version->len) == false) {
			goto error;
		}
	}

	if (generator->url->len != 0) {
		if (str->len != 0) {
			if (catcs(str, ' ') == false) {
				goto error;
			}
		}
		if (catas(str, generator->url->ptr, generator->url->len) == false) {
			goto error;
		}
	}

	return str;

error:
	free_string(str);
	return NULL;
}
