#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

#define WCHAR_IS_WHITESPACE(A) (((A) == L' ') || ((A) == L'\n') || ((A) == L'\t'))
#define MAX_TAG_NAME_SIZE 1000

static bool
expand_atts(struct xml_attribute **atts, size_t length)
{
	struct xml_attribute *temp = realloc(*atts, sizeof(struct xml_attribute) * length);
	if (temp == NULL) {
		return false;
	}
	*atts = temp;
	(*atts)[length - 1].name = NULL;
	(*atts)[length - 1].value = NULL;
	return true;
}

static void
free_atts(struct xml_attribute *atts, size_t length)
{
	for (size_t i = 0; i < length; ++i) {
		free_wstring(atts[i].name);
		free_wstring(atts[i].value);
	}
	free(atts);
}

// On success returns array of xml_attribute structures:
// * first structure has special meaning:
//   - its name is set to tag name;
// * last structure is special too:
//   - its name is set to NULL.
// On failure returns NULL.
// TODO THIS IS JUST AWFUL, REWRITE, REWRITE, REWRITE TODO
struct xml_attribute *
get_attribute_list_of_xml_tag(const struct wstring *tag)
{
	INFO("Trying to break down <%ls>", tag->ptr);

	struct xml_attribute *atts = NULL;
	size_t atts_len = 1;
	size_t atts_index;

	if (expand_atts(&atts, atts_len) == false) {
		free_atts(atts, atts_len - 1);
		return NULL;
	}

	wchar_t tag_name[MAX_TAG_NAME_SIZE];
	size_t tag_name_len = 0;

	wchar_t word[1000];
	size_t word_len = 0;

	bool in_tag_name = true;
	bool in_attribute_name = false;
	bool in_attribute_value = false;
	bool quoted_value = false;
	bool single_quoted_value = false;

	for (size_t i = 0; i < tag->len; ++i) {
		if (in_attribute_value == true) {
			if (tag->ptr[i] == L'"') {
				if ((word_len == 0) &&
				    (quoted_value == false) &&
				    (single_quoted_value == false))
				{
					quoted_value = true;
				} else if (quoted_value == true) {
					in_attribute_value = false;
					quoted_value = false;
					atts[atts_index].value = create_wstring(word, word_len);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						return NULL;
					}
					word_len = 0;
				} else {
					word[word_len++] = L'"';
				}
			} else if (tag->ptr[i] == L'\'') {
				if ((word_len == 0) &&
				    (single_quoted_value == false) &&
				    (quoted_value == false))
				{
					single_quoted_value = true;
				} else if (single_quoted_value == true) {
					in_attribute_value = false;
					single_quoted_value = false;
					atts[atts_index].value = create_wstring(word, word_len);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						return NULL;
					}
					word_len = 0;
				} else {
					word[word_len++] = L'\'';
				}
			} else if (WCHAR_IS_WHITESPACE(tag->ptr[i])) {
				if (quoted_value || single_quoted_value) {
					word[word_len++] = tag->ptr[i];
				} else {
					in_attribute_value = false;
					atts[atts_index].value = create_wstring(word, word_len);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						return NULL;
					}
					word_len = 0;
				}
			} else {
				word[word_len++] = tag->ptr[i];
			}
		} else if (in_attribute_name == true) {
			if ((WCHAR_IS_WHITESPACE(tag->ptr[i])) || (tag->ptr[i] == L'=')) {
				if (word_len == 0) {
					continue;
				}
				atts_index = atts_len++;
				if (expand_atts(&atts, atts_len) == false) {
					free_atts(atts, atts_len - 1);
					return NULL;
				}
				atts[atts_index].name = create_wstring(word, word_len);
				if (atts[atts_index].name == NULL) {
					free_atts(atts, atts_len);
					return NULL;
				}
				if (tag->ptr[i] == L'=') {
					word_len = 0;
					in_attribute_value = true;
				}
			} else {
				word[word_len++] = tag->ptr[i];
			}
		} else if (in_tag_name == true) {
			if (WCHAR_IS_WHITESPACE(tag->ptr[i])) {
				word_len = 0;
				in_tag_name = false;
				in_attribute_name = true;
			} else {
				if (tag_name_len == MAX_TAG_NAME_SIZE) {
					free(atts);
					return NULL;
				} else {
					tag_name[tag_name_len++] = tag->ptr[i];
				}
			}
		}
	}

	if (word_len != 0) {
		if (in_attribute_value == true) {
			atts[atts_index].value = create_wstring(word, word_len);
			if (atts[atts_index].value == NULL) {
				free_atts(atts, atts_len);
				return NULL;
			}
		} else if (in_attribute_name == true) {
			atts_index = atts_len++;
			if (expand_atts(&atts, atts_len) == false) {
				free_atts(atts, atts_len - 1);
				return NULL;
			}
			atts[atts_index].name = create_wstring(word, word_len);
			if (atts[atts_index].name == NULL) {
				free_atts(atts, atts_len);
				return NULL;
			}
		}
	}

	tag_name[tag_name_len] = L'\0';
	atts[0].name = create_wstring(tag_name, tag_name_len);
	if (atts[0].name == NULL) {
		free_atts(atts, atts_len);
		return NULL;
	}

	atts_index = atts_len++;
	if (expand_atts(&atts, atts_len) == false) {
		free_atts(atts, atts_len - 1);
		return NULL;
	}
	atts[atts_index].name = NULL;

	INFO("- name of tag is \"%ls\"", tag_name);
	for (size_t i = 1; i < atts_len - 1; ++i) {
		INFO("-- value of \"%ls\" attribute is \"%ls\"",
		     atts[i].name ? atts[i].name->ptr : L"(null)",
		     atts[i].value ? atts[i].value->ptr : L"(null)");
	}

	return atts;
}

const struct wstring *
get_value_of_xml_attribute(const struct xml_attribute *atts, const wchar_t *attr)
{
	size_t i = 1;
	while (atts[i].name != NULL) {
		if (wcscmp(atts[i].name->ptr, attr) == 0) {
			return atts[i].value;
		}
	}
	return NULL;
}

void
free_attribute_list_of_xml_tag(struct xml_attribute *atts)
{
	size_t i = 0;
	while (atts[i].name != NULL) {
		free_wstring(atts[i].name);
		free_wstring(atts[i].value);
		++i;
	}
	free(atts);
}
