#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static bool
expand_atts_of_tag(struct xml_tag *tag)
{
	struct xml_attribute *temp = realloc(tag->atts, sizeof(struct xml_attribute) * (tag->atts_len + 1));
	if (temp == NULL) {
		return false;
	}
	temp[tag->atts_len].name = wcrtes();
	temp[tag->atts_len].value = wcrtes();
	tag->atts = temp;
	++(tag->atts_len);
	return true;
}

struct xml_tag *
create_tag(void)
{
	struct xml_tag *tag = malloc(sizeof(struct xml_tag));
	if (tag == NULL) {
		FAIL("Not enough memory for XML tag structure!");
		return NULL;
	}
	tag->buf = wcrtes();
	if (tag->buf == NULL) {
		FAIL("Not enough memory for XML tag buffer!");
		free(tag);
		return NULL;
	}
	tag->atts = NULL;
	tag->atts_len = 0;
	if (expand_atts_of_tag(tag) == false) {
		FAIL("Not enough memory for XML tag attributes!");
		free_wstring(tag->buf);
		free(tag);
		return NULL;
	}
	tag->pos = XML_TAG_ATTRIBUTE_NAME;
	return tag;
}

enum xml_tag_status
append_wchar_to_tag(struct xml_tag *tag, wchar_t wc)
{
	if (wc == L'>') {
		if ((tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) ||
		    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED))
		{
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		} else {
			if (tag->atts[tag->atts_len - 1].name->len == 0) {
				free_wstring(tag->atts[tag->atts_len - 1].name);
				free_wstring(tag->atts[tag->atts_len - 1].value);
				--(tag->atts_len);
			}
			return XML_TAG_DONE;
		}
	} else if (ISWIDEWHITESPACE(wc)) {
		if ((tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) ||
		    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED))
		{
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		} else if (tag->pos == XML_TAG_ATTRIBUTE_NAME) {
			if (tag->atts[tag->atts_len - 1].name->len != 0) {
				expand_atts_of_tag(tag);
			}
		} else if (tag->pos == XML_TAG_ATTRIBUTE_VALUE_START) {
			tag->pos = XML_TAG_ATTRIBUTE_NAME;
			expand_atts_of_tag(tag);
		}
	} else if (wc == L'"') {
		if (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED) {
			tag->pos = XML_TAG_ATTRIBUTE_NAME;
		} else if (tag->pos == XML_TAG_ATTRIBUTE_VALUE_START) {
			tag->pos = XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED;
		} else if (XML_TAG_ATTRIBUTE_VALUE_QUOTED) {
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		}
	} else if (wc == L'\'') {
		if (tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) {
			tag->pos = XML_TAG_ATTRIBUTE_NAME;
		} else if (tag->pos == XML_TAG_ATTRIBUTE_VALUE_START) {
			tag->pos = XML_TAG_ATTRIBUTE_VALUE_QUOTED;
		} else if (XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED) {
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		}
	} else if (wc == L'=') {
		if ((tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) ||
		    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED))
		{
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		} else if (tag->pos == XML_TAG_ATTRIBUTE_NAME) {
			tag->pos = XML_TAG_ATTRIBUTE_VALUE_START;
		}
	} else {
		if ((tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) ||
		    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED) ||
		    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_START))
		{
			wcatcs(tag->atts[tag->atts_len - 1].value, wc);
		} else if (tag->pos == XML_TAG_ATTRIBUTE_NAME) {
			wcatcs(tag->atts[tag->atts_len - 1].name, wc);
		}
	}

	if (wcatcs(tag->buf, wc) == false) {
		return XML_TAG_FAIL;
	}

	return XML_TAG_CONTINUE;
}

bool
append_array_to_tag(struct xml_tag *tag, const wchar_t *src, size_t src_len)
{
	if ((tag->pos == XML_TAG_ATTRIBUTE_VALUE_START) ||
	    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_QUOTED) ||
	    (tag->pos == XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED))
	{
		return wcatas(tag->atts[tag->atts_len - 1].value, src, src_len);
	}
	return true;
}

const struct wstring *
get_value_of_xml_attribute(const struct xml_tag *tag, const wchar_t *attr)
{
	for (size_t i = 0; i < tag->atts_len; ++i) {
		if (wcscmp(tag->atts[i].name->ptr, attr) == 0) {
			return (const struct wstring *)tag->atts[i].value;
		}
	}
	return NULL;
}

void
empty_tag(struct xml_tag *tag)
{
	empty_wstring(tag->buf);
	for (size_t i = 1; i < tag->atts_len; ++i) {
		free_wstring(tag->atts[i].name);
		free_wstring(tag->atts[i].value);
	}
	tag->atts_len = 1;
	empty_wstring(tag->atts[0].name);
	empty_wstring(tag->atts[0].value);
}

void
free_tag(struct xml_tag *tag)
{
	if (tag == NULL) {
		return;
	}
	free_wstring(tag->buf);
	for (size_t i = 0; i < tag->atts_len; ++i) {
		free_wstring(tag->atts[i].name);
		free_wstring(tag->atts[i].value);
	}
	free(tag->atts);
	free(tag);
}
