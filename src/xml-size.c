#include <stdio.h>
#include "feedeater.h"

struct string *
convert_bytes_to_human_readable_size_string(const char *value)
{
	int bytes;
	if (sscanf(value, "%d", &bytes) != 1) {
		FAIL("Can not convert \"%s\" string from bytes to human readable format!", value);
		return NULL;
	}
	float size = bytes;
	int prefix = 0;
	while ((size > 100000) && (prefix < 3)) {
		size = size / 1000;
		++prefix;
	}
	char human_readable[30];
	int length;
	if (prefix == 1) {
		length = sprintf(human_readable, "%.2f KB", size);
	} else if (prefix == 2) {
		length = sprintf(human_readable, "%.2f MB", size);
	} else if (prefix == 0) {
		length = sprintf(human_readable, "%.2f bytes", size);
	} else {
		length = sprintf(human_readable, "%.2f GB", size);
	}
	return create_string(human_readable, length);
}
