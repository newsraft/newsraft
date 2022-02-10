#include <string.h>
#include "feedeater.h"

bool
check_url_for_validity(const struct string *str)
{
	if (str == NULL) {
		fprintf(stderr, "URL is NULL!\n");
		return false;
	}
	if (str->len == 0) {
		fprintf(stderr, "URL is empty!\n");
		return false;
	}
	if (strstr(str->ptr, "://") == NULL) {
		fprintf(stderr, "Missing protocol scheme of the URL!\n");
		fprintf(stderr, "Every URL must start with a protocol definition like \"http://\".\n");
		return false;
	}
	if ((strncmp(str->ptr, "http://",  7) != 0) &&
	    (strncmp(str->ptr, "https://", 8) != 0) &&
	    (strncmp(str->ptr, "file://",  7) != 0) &&
	    (strncmp(str->ptr, "ftp://",   6) != 0))
	{
		fprintf(stderr, "Unknown protocol scheme of the URL!\n");
		fprintf(stderr, "Supported protocols are http, https, file, ftp.\n");
		return false;
	}
	return true;
}
