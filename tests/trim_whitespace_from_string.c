#include <string.h>
#include "newsraft.h"

int
main(void)
{
	struct string *str = crtes(100);

	cpyas(str, "The Devil's Advocate", 20);
	trim_whitespace_from_string(str);
	if (strcmp(str->ptr, "The Devil's Advocate") != 0) goto error;

	cpyas(str, "      The Terminal", 18);
	trim_whitespace_from_string(str);
	if (strcmp(str->ptr, "The Terminal") != 0) goto error;

	cpyas(str, "Black Swan    ", 14);
	trim_whitespace_from_string(str);
	if (strcmp(str->ptr, "Black Swan") != 0) goto error;

	cpyas(str, "     El hoyo       ", 19);
	trim_whitespace_from_string(str);
	if (strcmp(str->ptr, "El hoyo") != 0) goto error;

	cpyas(str, " The Departed ", 14);
	trim_whitespace_from_string(str);
	if (strcmp(str->ptr, "The Departed") != 0) goto error;

	free_string(str);
	return 0;
error:
	free_string(str);
	return 1;
}
