#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Create string out of array.
// On success returns pointer to string.
// On memory shortage returns NULL.
struct string *
crtas(const char *src_ptr, size_t src_len)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		FAIL("Not enough memory for string structure!");
		return NULL;
	}
	size_t new_lim = src_len * 2; // Multiply by 2 to decrease number of further realloc calls.
	str->ptr = malloc(sizeof(char) * (new_lim + 1));
	if (str->ptr == NULL) {
		FAIL("Not enough memory for string pointer!");
		free(str);
		return NULL;
	}
	memcpy(str->ptr, src_ptr, sizeof(char) * src_len);
	*(str->ptr + src_len) = '\0';
	str->len = src_len;
	str->lim = new_lim;
	return str;
}

// Create string out of string.
struct string *
crtss(const struct string *src)
{
	return crtas(src->ptr, src->len);
}

// Create empty string.
struct string *
crtes(void)
{
	return crtas("", 0);
}

// Copy array to string.
// On success returns true.
// On memory shortage returns false.
bool
cpyas(struct string *dest, const char *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		size_t new_lim = src_len * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	memcpy(dest->ptr, src_ptr, sizeof(char) * src_len);
	*(dest->ptr + src_len) = '\0';
	dest->len = src_len;
	return true;
}

// Copy string to string.
bool
cpyss(struct string *dest, const struct string *src)
{
	return cpyas(dest, src->ptr, src->len);
}

// Concatenate array to string.
// On success returns true.
// On memory shortage returns false.
bool
catas(struct string *dest, const char *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		size_t new_lim = new_len * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	memcpy(dest->ptr + dest->len, src_ptr, sizeof(char) * src_len);
	*(dest->ptr + new_len) = '\0';
	dest->len = new_len;
	return true;
}

// Concatenate string to string.
bool
catss(struct string *dest, const struct string *src)
{
	return catas(dest, src->ptr, src->len);
}

// Concatenate character to string.
// On success returns true.
// On memory shortage returns false.
bool
catcs(struct string *dest, char c)
{
	char src[1] = {c};
	return catas(dest, src, 1);
}

bool
crtas_or_cpyas(struct string **dest, const char *src_ptr, size_t src_len)
{
	if (*dest != NULL) {
		return cpyas(*dest, src_ptr, src_len);
	}
	*dest = crtas(src_ptr, src_len);
	if (*dest == NULL) {
		return false;
	}
	return true;
}

bool
crtss_or_cpyss(struct string **dest, const struct string *src)
{
	if (*dest != NULL) {
		return cpyss(*dest, src);
	}
	*dest = crtss(src);
	if (*dest == NULL) {
		return false;
	}
	return true;
}

bool
string_vprintf(struct string *dest, const char *format, va_list args)
{
	int required_length = vsnprintf(dest->ptr, 0, format, args);
	if (required_length < 0) {
		return false;
	}
	// We already know that integer is positive, so it is safe to cast it to unsigned integer.
	if ((size_t)required_length > dest->lim) {
		size_t new_lim = (size_t)required_length * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for printing to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	required_length = vsnprintf(dest->ptr, required_length + 1, format, args);
	if (required_length < 0) {
		empty_string(dest);
		return false;
	}
	dest->len = (size_t)required_length;
	*(dest->ptr + dest->len) = '\0';
	return true;
}

bool
string_printf(struct string *dest, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	bool result = string_vprintf(dest, format, args);
	va_end(args);
	return result;
}

void
empty_string(struct string *str)
{
	str->len = 0;
	*(str->ptr + 0) = '\0';
}

void
free_string(struct string *str)
{
	if (str != NULL) {
		free(str->ptr);
		free(str);
	}
}

void
trim_whitespace_from_string(struct string *str)
{
	if (str->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = str->len - 1;
	while (ISWHITESPACE(*(str->ptr + left_edge)) && left_edge <= right_edge) {
		++left_edge;
	}
	while (ISWHITESPACE(*(str->ptr + right_edge)) && right_edge >= left_edge) {
		--right_edge;
	}

	if ((left_edge == 0) && (right_edge == (str->len - 1))) {
		return;
	}

	if (right_edge < left_edge) {
		*(str->ptr + 0) = '\0';
		str->len = 0;
		return;
	}

	size_t trimmed_string_len = right_edge - left_edge + 1;
	for (size_t i = 0; i < trimmed_string_len; ++i) {
		*(str->ptr + i) = *(str->ptr + i + left_edge);
	}
	str->len = trimmed_string_len;
	*(str->ptr + trimmed_string_len) = '\0';
}

// On failure retruns NULL.
struct wstring *
convert_string_to_wstring(const struct string *src)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		return NULL;
	}
	wstr->len = mbstowcs(NULL, src->ptr, 0);
	if (wstr->len == (size_t)-1) {
		free(wstr);
		return NULL;
	}
	wstr->ptr = malloc(sizeof(wchar_t) * (wstr->len + 1));
	if (wstr->ptr == NULL) {
		free(wstr);
		return NULL;
	}
	if (mbstowcs(wstr->ptr, src->ptr, wstr->len + 1) == (size_t)-1) {
		free(wstr->ptr);
		free(wstr);
		return NULL;
	}
	wstr->ptr[wstr->len] = L'\0';
	wstr->lim = wstr->len;
	return wstr;
}

void
remove_character_from_string(struct string *str, char c)
{
	for (char *i = str->ptr; *i != '\0'; ++i) {
		if (*i == c) {
			for (char *j = i; *j != '\0'; ++j) {
				*j = *(j + 1);
			}
			str->len -= 1;
			i -= 1;
		}
	}
}

void
remove_trailing_slashes_from_string(struct string *str)
{
	while ((str->len > 0) && (str->ptr[str->len - 1] == '/')) {
		str->len -= 1;
	}
	str->ptr[str->len] = '\0';
}

struct string *
convert_bytes_to_human_readable_size_string(const char *value)
{
	float size;
	if (sscanf(value, "%f", &size) != 1) {
		FAIL("Can't convert \"%s\" string to float!", value);
		return NULL;
	}
	if (size < 0) {
		FAIL("With some fright, the number of bytes turned out to be negative!");
		return NULL;
	}
	uint8_t prefix = 0;
	const size_t conversion_threshold = get_cfg_uint(CFG_SIZE_CONVERSION_THRESHOLD);
	while ((size > conversion_threshold) && (prefix < 3)) {
		size = size / 1000;
		++prefix;
	}
	// longest float integral part (40) +
	// point (1) +
	// two digits after point (2) +
	// space (1) +
	// longest name of data measure (5) +
	// null terminator (1) +
	// for luck lol (1) =
	// 51
	char human_readable[51];
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
	return crtas(human_readable, length);
}

struct string *
convert_seconds_to_human_readable_duration_string(const char *value)
{
	float duration;
	if (sscanf(value, "%f", &duration) != 1) {
		FAIL("Can't convert \"%s\" string to float!", value);
		return NULL;
	}
	if (duration < 0) {
		FAIL("With some fright, the number of seconds turned out to be negative!");
		return NULL;
	}
	uint8_t prefix = 0;
	if (duration > 90) {
		duration = duration / 60;
		prefix = 1;
		if (duration > 90) {
			duration = duration / 60;
			prefix = 2;
		}
	}
	// longest float integral part (40) +
	// point (1) +
	// one digit after point (1) +
	// space (1) +
	// longest name of data measure (7) +
	// null terminator (1) +
	// for luck lol (1) =
	// 52
	char human_readable[52];
	int length;
	if (prefix == 1) {
		length = sprintf(human_readable, "%.1f minutes", duration);
	} else if (prefix == 2) {
		length = sprintf(human_readable, "%.1f hours", duration);
	} else {
		length = sprintf(human_readable, "%.0f seconds", duration);
	}
	return crtas(human_readable, length);
}
