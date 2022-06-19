#include <string.h>
#include "newsraft.h"

bool
check_url_for_validity(const struct string *str)
{
	if (str == NULL) {
		fputs("URL is null!\n", stderr);
		return false;
	}
	if (str->len == 0) {
		fputs("URL is empty!\n", stderr);
		return false;
	}
	if (strstr(str->ptr, "://") == NULL) {
		fputs("Missing protocol scheme of the URL!\n", stderr);
		fputs("Every URL must start with a protocol definition like \"http://\".\n", stderr);
		return false;
	}
	if ((strncmp(str->ptr, "http://", 7) != 0)
			&& (strncmp(str->ptr, "https://", 8) != 0)
			&& (strncmp(str->ptr, "ftp://", 6) != 0)
			&& (strncmp(str->ptr, "file://", 7) != 0))
	{
		fputs("Unknown protocol scheme of the URL!\n", stderr);
		fputs("Supported protocols are http, https, ftp and file.\n", stderr);
		return false;
	}
	return true;
}
