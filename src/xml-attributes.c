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

	struct wstring *word = wcrtes();
	if (word == NULL) {
		return NULL;
	}

	struct xml_attribute *atts = NULL;
	size_t atts_len = 1;
	size_t atts_index;

	if (expand_atts(&atts, atts_len) == false) {
		free_atts(atts, atts_len - 1);
		free_wstring(word);
		return NULL;
	}

	wchar_t tag_name[MAX_TAG_NAME_SIZE];
	size_t tag_name_len = 0;

	bool in_tag_name = true;
	bool in_attribute_name = false;
	bool in_attribute_value = false;
	bool quoted_value = false;
	bool single_quoted_value = false;
	bool in_entity = false;
	wchar_t entity_name[MAX_ENTITY_NAME_LENGTH + 1];
	const wchar_t *entity_value;
	size_t entity_len;

	const wchar_t *iter = tag->ptr;
	while (*iter != L'\0') {

		if (*iter == L'&') {
			in_entity = true;
			entity_len = 0;
			++iter;
			continue;
		} else if (in_entity == true) {
			if (*iter == L';') {
				in_entity = false;
				entity_name[entity_len] = L'\0';
				entity_value = translate_html_entity(entity_name);
				if (entity_value != NULL) {
					wcatas(word, entity_value, wcslen(entity_value));
				} else {
					wcatcs(word, L'&');
					wcatas(word, entity_name, entity_len);
					wcatcs(word, L';');
				}
			} else {
				if (entity_len == MAX_ENTITY_NAME_LENGTH) {
					in_entity = false;
					entity_name[entity_len] = L'\0';
					wcatcs(word, L'&');
					wcatas(word, entity_name, entity_len);
				} else {
					entity_name[entity_len++] = *iter;
				}
			}
			++iter;
			continue;
		}

		if (in_attribute_value == true) {

			if (*iter == L'"') {
				if ((word->len == 0) &&
				    (quoted_value == false) &&
				    (single_quoted_value == false))
				{
					quoted_value = true;
				} else if (quoted_value == true) {
					in_attribute_value = false;
					quoted_value = false;
					atts[atts_index].value = wcrtss(word);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						free_wstring(word);
						return NULL;
					}
					empty_wstring(word);
				} else {
					wcatcs(word, L'"');
				}
			} else if (*iter == L'\'') {
				if ((word->len == 0) &&
				    (single_quoted_value == false) &&
				    (quoted_value == false))
				{
					single_quoted_value = true;
				} else if (single_quoted_value == true) {
					in_attribute_value = false;
					single_quoted_value = false;
					atts[atts_index].value = wcrtss(word);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						free_wstring(word);
						return NULL;
					}
					empty_wstring(word);
				} else {
					wcatcs(word, L'\'');
				}
			} else if (WCHAR_IS_WHITESPACE(*iter)) {
				if (quoted_value || single_quoted_value) {
					wcatcs(word, *iter);
				} else {
					in_attribute_value = false;
					atts[atts_index].value = wcrtss(word);
					if (atts[atts_index].value == NULL) {
						free_atts(atts, atts_len);
						free_wstring(word);
						return NULL;
					}
					empty_wstring(word);
				}
			} else {
				wcatcs(word, *iter);
			}

		} else if (in_attribute_name == true) {

			if ((WCHAR_IS_WHITESPACE(*iter)) || (*iter == L'=')) {
				if (word->len == 0) {
					++iter;
					continue;
				}
				atts_index = atts_len++;
				if (expand_atts(&atts, atts_len) == false) {
					free_atts(atts, atts_len - 1);
					free_wstring(word);
					return NULL;
				}
				atts[atts_index].name = wcrtss(word);
				if (atts[atts_index].name == NULL) {
					free_atts(atts, atts_len);
					free_wstring(word);
					return NULL;
				}
				if (*iter == L'=') {
					empty_wstring(word);
					in_attribute_value = true;
				}
			} else {
				wcatcs(word, *iter);
			}

		} else if (in_tag_name == true) {

			if (WCHAR_IS_WHITESPACE(*iter)) {
				empty_wstring(word);
				in_tag_name = false;
				in_attribute_name = true;
			} else {
				if (tag_name_len == MAX_TAG_NAME_SIZE) {
					free(atts);
					free_wstring(word);
					return NULL;
				} else {
					tag_name[tag_name_len++] = *iter;
				}
			}

		}

		++iter;
	}

	if (word->len != 0) {
		if (in_attribute_value == true) {
			atts[atts_index].value = wcrtss(word);
			if (atts[atts_index].value == NULL) {
				free_atts(atts, atts_len);
				free_wstring(word);
				return NULL;
			}
		} else if (in_attribute_name == true) {
			atts_index = atts_len++;
			if (expand_atts(&atts, atts_len) == false) {
				free_atts(atts, atts_len - 1);
				free_wstring(word);
				return NULL;
			}
			atts[atts_index].name = wcrtss(word);
			if (atts[atts_index].name == NULL) {
				free_atts(atts, atts_len);
				free_wstring(word);
				return NULL;
			}
		}
	}

	free_wstring(word);

	tag_name[tag_name_len] = L'\0';
	atts[0].name = wcrtas(tag_name, tag_name_len);
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
		++i;
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
