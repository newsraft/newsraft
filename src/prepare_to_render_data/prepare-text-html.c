#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <gumbo.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

struct html_data {
	struct links_list *links;
	struct string **abbrs;
	size_t abbrs_len;
	bool in_pre_element;
	size_t list_depth;
};

struct html_element_preparer {
	const GumboTag tag;
	void (*start_handler)(struct string *, struct html_data *, GumboVector *);
	void (*end_handler)(struct string *, struct html_data *, GumboVector *);
	const uint8_t newlines_before;
	const uint8_t newlines_after;
	const char *prefix;
	const char *suffix;
};

static int
count_how_many_newlines_there_are_at_this_position(const struct string *text)
{
	bool in_tag = false;
	int count = 0;
	size_t i = text->len, close_pos = 0; // Initialize to shut the compiler.
	while (i > 0) {
		if (ISWHITESPACE(text->ptr[i - 1])) {
			// nop
		} else if (text->ptr[i - 1] == '>') {
			close_pos = i - 1;
			in_tag = true;
		} else if (text->ptr[i - 1] == '<' && in_tag == true) {
			size_t tag_name_pos = i;
			size_t tag_name_len = close_pos - tag_name_pos;
			if (tag_name_len == 2) {
				if (text->ptr[tag_name_pos] == 'b' && text->ptr[tag_name_pos + 1] == 'r') {
					count += 1;
				} else if (text->ptr[tag_name_pos] == 'l' && text->ptr[tag_name_pos + 1] == 'i') {
					return 9999; // Don't pollute beginning of list entries.
				} else if (text->ptr[tag_name_pos] == 'h' && text->ptr[tag_name_pos + 1] == 'r') {
					break; // <hr> will be expanded to text during rendering.
				}
			}
			in_tag = false;
		} else if (in_tag == false) {
			break;
		}
		i -= 1;
	}
	return i == 0 ? 9999 : count;
}

static void
provide_newlines(struct string *text, int count)
{
	int newlines = count_how_many_newlines_there_are_at_this_position(text);
	for (int i = 0; i < (count - newlines); ++i) {
		catas(text, "<br>", 4);
	}
}

static const char *
get_value_of_xml_attribute(GumboVector *attrs, const char *attr_name)
{
	GumboAttribute *attr = gumbo_get_attribute(attrs, attr_name);
	return attr != NULL ? attr->value : NULL;
}

static void
pre_in_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	(void)text;
	(void)attrs;
	data->in_pre_element = true;
}

static void
pre_out_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	(void)text;
	(void)attrs;
	data->in_pre_element = false;
}

static void
add_url_mark(struct links_list *links, struct string *text, const char *url, const char *type, const char *title)
{
	if (url == NULL) {
		return; // Ignore empty links.
	}
	if (url[0] == '#') {
		return; // Ignore anchors to elements.
	}
	size_t url_len = strlen(url);
	if (url_len == 0) {
		return; // Ignore empty links.
	}
	int64_t url_index = add_another_url_to_trim_links_list(links, url, url_len);
	if (url_index < 0) {
		return; // Ignore invalid links.
	}
	char index[100];
	int index_len = snprintf(index, 100, "[%" PRId64, url_index + 1);
	if (index_len < 2 || index_len > 99) {
		return; // Should never happen.
	}
	// Prepend &nbsp; instead of space to avoid line wrapping of URL mark alone.
	if (count_how_many_newlines_there_are_at_this_position(text) == 0) {
		catas(text, "&nbsp;", 6);
	}
	catas(text, index, index_len);
	if (type != NULL || title != NULL) {
		catas(text, ", ", 2);
		if (type != NULL) {
			catas(text, type, strlen(type));
		}
		if (title != NULL) {
			type == NULL ? catas(text, "\"", 1) : catas(text, " \"", 2);
			catas(text, title, strlen(title));
			catas(text, "\"", 1);
		}
	}
	catas(text, "]", 1);
}

static void
a_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "href");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	add_url_mark(data->links, text, url, type, title != NULL && strlen(title) == 0 ? NULL : title);
}

static void
img_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	if ((title == NULL) || (strlen(title) == 0)) {
		title = get_value_of_xml_attribute(attrs, "alt");
	}
	add_url_mark(data->links, text, url, "image", title != NULL && strlen(title) == 0 ? NULL : title);
}

static void
iframe_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	if ((title == NULL) || (strlen(title) == 0)) {
		title = get_value_of_xml_attribute(attrs, "name");
	}
	add_url_mark(data->links, text, url, "iframe", title != NULL && strlen(title) == 0 ? NULL : title);
}

static void
embed_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	add_url_mark(data->links, text, url, type == NULL ? "embed" : type, NULL);
}

static void
audio_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	add_url_mark(data->links, text, url, "audio", NULL);
}

static void
video_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	add_url_mark(data->links, text, url, "video", NULL);
}

static void
source_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	add_url_mark(data->links, text, url, type, NULL);
}

static void
object_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "data");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	const char *title = get_value_of_xml_attribute(attrs, "name");
	add_url_mark(data->links, text, url, type, title);
}

static void
abbr_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	const char *title = get_value_of_xml_attribute(attrs, "title");
	if (title == NULL) {
		return;
	}
	const size_t title_len = strlen(title);
	if (title_len == 0) {
		return;
	}
	for (size_t i = 0; i < data->abbrs_len; ++i) {
		if (strcasecmp(title, data->abbrs[i]->ptr) == 0) {
			return; // It's a duplicate.
		}
	}
	catas(text, " (", 2);
	catas(text, title, title_len);
	catcs(text, ')');
	void *tmp = realloc(data->abbrs, sizeof(struct string *) * (data->abbrs_len + 1));
	if (tmp != NULL) {
		data->abbrs = tmp;
		data->abbrs[data->abbrs_len] = crtas(title, title_len);
		if (data->abbrs[data->abbrs_len] != NULL) {
			data->abbrs_len += 1;
		}
	}
}

static void
ul_in_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	(void)attrs;
	provide_newlines(text, data->list_depth == 0 ? 2 : 1);
	data->list_depth += 1;
}

static void
ul_out_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	(void)attrs;
	if (data->list_depth > 0) data->list_depth -= 1;
	provide_newlines(text, data->list_depth == 0 ? 2 : 1);
}

static void
input_handler(struct string *text, struct html_data *data, GumboVector *attrs)
{
	(void)data;
	const char *type = get_value_of_xml_attribute(attrs, "type");
	if (type == NULL || strcmp(type, "hidden") == 0) {
		return;
	}
	const char *value = get_value_of_xml_attribute(attrs, "value");
	if (value == NULL) {
		return;
	}
	catcs(text, '[');
	catas(text, value, strlen(value));
	catcs(text, ']');
}

static const struct html_element_preparer preparers[] = {
	{GUMBO_TAG_P,          NULL,            NULL,             2, 2, NULL,   NULL},
	{GUMBO_TAG_SPAN,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_A,          NULL,            &a_handler,       0, 0, "<u>",  "</u>"},
	{GUMBO_TAG_LI,         NULL,            NULL,             1, 1, "<li>", "</li>"},
	{GUMBO_TAG_UL,         &ul_in_handler,  &ul_out_handler,  0, 0, "<ul>", "</ul>"},
	{GUMBO_TAG_OL,         &ul_in_handler,  &ul_out_handler,  0, 0, "<ol>", "</ol>"},
	{GUMBO_TAG_PRE,        &pre_in_handler, &pre_out_handler, 1, 1, NULL,   NULL},
	{GUMBO_TAG_HR,         NULL,            NULL,             1, 1, "<hr>", NULL},
	{GUMBO_TAG_FIGURE,     NULL,            NULL,             2, 2, "<figure>", "</figure>"},
	{GUMBO_TAG_BLOCKQUOTE, NULL,            NULL,             2, 2, "<figure>", "</figure>"},
	{GUMBO_TAG_DD,         NULL,            NULL,             1, 1, "<figure>", "</figure>"},
	{GUMBO_TAG_H1,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_H2,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_H3,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_H4,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_H5,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_H6,         NULL,            NULL,             3, 2, "<b>",  "</b>"},
	{GUMBO_TAG_HEADER,     NULL,            NULL,             3, 2, NULL,   NULL},
	{GUMBO_TAG_STRONG,     NULL,            NULL,             0, 0, "<b>",  "</b>"},
	{GUMBO_TAG_MARK,       NULL,            NULL,             0, 0, "<b>",  "</b>"},
	{GUMBO_TAG_BIG,        NULL,            NULL,             0, 0, "<b>",  "</b>"},
	{GUMBO_TAG_ADDRESS,    NULL,            NULL,             1, 1, "<i>",  "</i>"},
	{GUMBO_TAG_EM,         NULL,            NULL,             0, 0, "<i>",  "</i>"},
	{GUMBO_TAG_VAR,        NULL,            NULL,             0, 0, "<i>",  "</i>"},
	{GUMBO_TAG_SMALL,      NULL,            NULL,             0, 0, "<i>",  "</i>"},
	{GUMBO_TAG_DFN,        NULL,            NULL,             0, 0, "<i>",  "</i>"},
	{GUMBO_TAG_INS,        NULL,            NULL,             0, 0, "<u>",  "</u>"},
	{GUMBO_TAG_IMG,        &img_handler,    NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_IFRAME,     &iframe_handler, NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_EMBED,      &embed_handler,  NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_SOURCE,     &source_handler, NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_OBJECT,     &object_handler, NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_ABBR,       NULL,            &abbr_handler,    0, 0, NULL,   NULL},
	{GUMBO_TAG_SUP,        NULL,            NULL,             0, 0, "^",    NULL},
	{GUMBO_TAG_Q,          NULL,            NULL,             0, 0, "\"",   "\""},
	{GUMBO_TAG_BUTTON,     NULL,            NULL,             0, 0, "[",    "]"},
	//{GUMBO_TAG_PICTURE,    NULL,            NULL,             0, 0, NULL,   NULL}, // Gumbo lacks it.
	{GUMBO_TAG_AUDIO,      &audio_handler,  NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_VIDEO,      &video_handler,  NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_INPUT,      &input_handler,  NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_DIV,        NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_CENTER,     NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_MAIN,       NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_ARTICLE,    NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_SUMMARY,    NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_FIGCAPTION, NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_SECTION,    NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_FOOTER,     NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_OPTION,     NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_FORM,       NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_ASIDE,      NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_NAV,        NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_HGROUP,     NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_DT,         NULL,            NULL,             1, 1, NULL,   NULL},
	{GUMBO_TAG_DL,         NULL,            NULL,             2, 2, NULL,   NULL},
	{GUMBO_TAG_DETAILS,    NULL,            NULL,             2, 2, NULL,   NULL},
	{GUMBO_TAG_LABEL,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_TEXTAREA,   NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_CODE,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_TT,         NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_SAMP,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_KBD,        NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_CITE,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_TIME,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_FONT,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_BASEFONT,   NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_THEAD,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_TBODY,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_TFOOT,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_WBR,        NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_NOSCRIPT,   NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_DATA,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_APPLET,     NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_PARAM,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_LINK,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_STYLE,      NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_SCRIPT,     NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_SVG,        NULL,            NULL,             0, 0, NULL,   " [svg image]"},
	{GUMBO_TAG_CANVAS,     NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_META,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_BODY,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_HEAD,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_HTML,       NULL,            NULL,             0, 0, NULL,   NULL},
	{GUMBO_TAG_UNKNOWN,    NULL,            NULL,             0, 0, NULL,   NULL},
};

static void
prepare_html(GumboNode *node, struct string *text, struct html_data *data)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i;
		for (i = 0; preparers[i].tag != GUMBO_TAG_UNKNOWN; ++i) {
			if (preparers[i].tag == node->v.element.tag) {
				break;
			}
		}
		if (preparers[i].tag == GUMBO_TAG_UNKNOWN) {
			catas(text, node->v.element.original_tag.data, node->v.element.original_tag.length);
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				prepare_html(node->v.element.children.data[j], text, data);
			}
			catas(text, node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
		} else {
			if (preparers[i].newlines_before > 0) {
				provide_newlines(text, preparers[i].newlines_before);
			}
			if (preparers[i].start_handler != NULL) {
				preparers[i].start_handler(text, data, &node->v.element.attributes);
			}
			if (preparers[i].prefix != NULL) {
				catas(text, preparers[i].prefix, strlen(preparers[i].prefix));
			}
			// Don't descend into elements which are illegible.
			if (preparers[i].tag != GUMBO_TAG_STYLE
				&& preparers[i].tag != GUMBO_TAG_SCRIPT
				&& preparers[i].tag != GUMBO_TAG_SVG
				&& preparers[i].tag != GUMBO_TAG_HEAD)
			{
				for (size_t j = 0; j < node->v.element.children.length; ++j) {
					prepare_html(node->v.element.children.data[j], text, data);
				}
			}
			if (preparers[i].suffix != NULL) {
				catas(text, preparers[i].suffix, strlen(preparers[i].suffix));
			}
			if (preparers[i].end_handler != NULL) {
				preparers[i].end_handler(text, data, &node->v.element.attributes);
			}
			if (preparers[i].newlines_after > 0) {
				provide_newlines(text, preparers[i].newlines_after);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT)
		|| (node->type == GUMBO_NODE_CDATA)
		|| (node->type == GUMBO_NODE_WHITESPACE))
	{
		if (data->in_pre_element == false) {
			catas(text, node->v.text.original_text.data, node->v.text.original_text.length);
		} else {
			for (size_t j = 0; j < node->v.text.original_text.length; ++j) {
				if (node->v.text.original_text.data[j] == ' ') {
					catas(text, "&nbsp;", 6);
				} else if (node->v.text.original_text.data[j] == '\n') {
					catas(text, "<br>", 4);
				} else if (node->v.text.original_text.data[j] == '\t') {
					catas(text, "&nbsp;&nbsp;&nbsp;&nbsp;", 24);
				} else {
					catcs(text, node->v.text.original_text.data[j]);
				}
			}
		}
	}
}

static inline void
free_abbrs(struct html_data *data)
{
	for (size_t i = 0; i < data->abbrs_len; ++i) {
		free_string(data->abbrs[i]);
	}
	free(data->abbrs);
}

struct wstring *
prepare_to_render_text_html(const struct wstring *wide_src, struct links_list *links)
{
	struct string *text = crtes(wide_src->len + 1000);
	if (text == NULL) {
		FAIL("Not enough memory for text buffer to prepare HTML render block!");
		return NULL;
	}
	struct string *src = convert_wstring_to_string(wide_src);
	if (src == NULL) {
		FAIL("Not enough memory for source buffer to prepare HTML render block!");
		free_string(text);
		return NULL;
	}
	GumboOutput *output = gumbo_parse(src->ptr);
	if (output == NULL) {
		FAIL("Couldn't parse HTML successfully!");
		free_string(text);
		free_string(src);
		return NULL;
	}
	struct html_data data = {links, NULL, 0, false, 0};
	prepare_html(output->root, text, &data);
	struct wstring *wtext = convert_string_to_wstring(text);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_abbrs(&data);
	free_string(src);
	free_string(text);
	return wtext;
}
