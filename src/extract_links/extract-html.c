#include "extract_links.h"

static inline bool
extract_links_from_attributes(const struct xml_attribute *atts, struct link_list *links)
{
	const struct wstring *value;
	if (wcscmp(atts[0].name->ptr, L"a") == 0) {
		value = get_value_of_xml_attribute(atts, L"href");
	} else if (wcscmp(atts[0].name->ptr, L"img") == 0) {
		value = get_value_of_xml_attribute(atts, L"src");
	} else if (wcscmp(atts[0].name->ptr, L"source") == 0) {
		value = get_value_of_xml_attribute(atts, L"src");
	} else if (wcscmp(atts[0].name->ptr, L"iframe") == 0) {
		value = get_value_of_xml_attribute(atts, L"src");
	} else {
		return true; // Not an error, just ignore tags without links.
	}
	if ((value == NULL) || (value->len == 0)) {
		return true; // Not an error, just ignore empty values.
	}
	struct string *str = convert_wstring_to_string(value);
	if (str == NULL) {
		return false;
	}
	bool status = add_another_url_to_trim_link_list(links, str->ptr, str->len);
	free_string(str);
	return status;
}

bool
extract_from_html(const struct wstring *wstr, struct link_list *links)
{
	struct wstring *tag = create_wstring(NULL, 100);
	if (tag == NULL) {
		FAIL("Not enough memory for tag buffer to extract links from HTML!");
		return false;
	}

	bool in_tag = false;
	struct xml_attribute *atts;
	bool success = true;

	const wchar_t *i = wstr->ptr;
	while (*i != L'\0') {
		if (in_tag == false) {
			if (*i == L'<') {
				in_tag = true;
				empty_wstring(tag);
			}
		} else {
			if (*i == L'>') {
				in_tag = false;
				tag->ptr[tag->len] = L'\0';
				atts = get_attribute_list_of_xml_tag(tag);
				if (atts != NULL) {
					if (extract_links_from_attributes(atts, links) == false) {
						free_attribute_list_of_xml_tag(atts);
						success = false;
						break;
					}
					free_attribute_list_of_xml_tag(atts);
				}
			} else {
				if (wcatcs(tag, *i) == false) {
					success = false;
					break;
				}
			}
		}
		++i;
			/* if (in_entity == false) { */
			/* 	if (*i == L'<') { */
			/* 		in_tag = true; */
			/* 		empty_wstring(tag); */
			/* 	} else if (*i == L'&') { */
			/* 		in_entity = true; */
			/* 		entity_len = 0; */
			/* 	} */
			/* } else { // All entity characters go here. */
			/* 	if (*i == L';') { */
			/* 		in_entity = false; */
			/* 		entity_name[entity_len] = L'\0'; */
			/* 		entity_value = translate_html_entity(entity_name); */
			/* 		if (entity_value != NULL) { */
			/* 			line_string(line, t, entity_value); */
			/* 		} else { */
			/* 			line_char(line, L'&', t); */
			/* 			line_string(line, t, entity_name); */
			/* 			line_char(line, L';', t); */
			/* 		} */
			/* 	} else { */
			/* 		if (entity_len == MAX_ENTITY_NAME_LENGTH) { */
			/* 			in_entity = false; */
			/* 			entity_name[entity_len] = L'\0'; */
			/* 			line_char(line, L'&', t); */
			/* 			line_string(line, t, entity_name); */
			/* 		} else { */
			/* 			entity_name[entity_len++] = *i; */
			/* 		} */
			/* 	} */
			/* } */
	}

	free_wstring(tag);
	return success;
}
