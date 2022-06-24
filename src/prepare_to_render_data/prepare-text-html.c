#include <gumbo.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

struct html_element_handler {
	const GumboTag tag_id;
	void (*start_handler)(struct string *, struct link_list *, GumboVector *);
	void (*end_handler)(struct string *, struct link_list *, GumboVector *);
};

static const char *
get_value_of_xml_attribute(GumboVector *attrs, const char *attr_name)
{
	GumboAttribute *attr = gumbo_get_attribute(attrs, attr_name);
	if (attr != NULL) {
		return attr->value;
	}
	return NULL;
}

static void
sup_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '^');
}

static void
q_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '"');
}

static void
button_start_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '[');
}

static void
button_end_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, ']');
}

static void
add_url_mark(struct string *text, const char *url, const char *title, size_t title_len, struct link_list *links, const char *data_type)
{
	if (url == NULL) {
		return;
	}
	size_t url_len = strlen(url);
	if (url_len == 0) {
		return;
	}

	if (url[0] == '#') {
		// Don't pollute the list of links with anchors to elements.
		INFO("Ignoring an anchor to element.");
		return;
	}

	int64_t url_index = add_another_url_to_trim_link_list(links, url, url_len);
	if (url_index < 0) {
		return;
	}

	// Add link mark to HTML content.
	struct string *url_mark = crtes();
	if (url_mark == NULL) {
		return;
	}

	if (data_type == NULL) {
		if ((title != NULL) && (title_len != 0)) {
			string_printf(url_mark, " [%" PRId64 ", \"%s\"]", url_index + 1, title);
		} else {
			string_printf(url_mark, " [%" PRId64 "]", url_index + 1);
		}
	} else {
		if ((title != NULL) && (title_len != 0)) {
			string_printf(url_mark, " [%" PRId64 ", %s \"%s\"]", url_index + 1, data_type, title);
		} else {
			string_printf(url_mark, " [%" PRId64 ", %s]", url_index + 1, data_type);
		}
	}

	catss(text, url_mark);
	free_string(url_mark);
}

static void
a_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "href");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	add_url_mark(text, url, title, title_len, links, type);
}

static void
img_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	if ((title == NULL) || (title_len == 0)) {
		title = get_value_of_xml_attribute(attrs, "alt");
		if (title != NULL) {
			title_len = strlen(title);
		}
	}
	add_url_mark(text, url, title, title_len, links, "image");
}

static void
iframe_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	if ((title == NULL) || (title_len == 0)) {
		title = get_value_of_xml_attribute(attrs, "name");
		if (title != NULL) {
			title_len = strlen(title);
		}
	}
	add_url_mark(text, url, title, title_len, links, "iframe");
}

static void
embed_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	if (type == NULL) {
		type = "embed";
	}
	add_url_mark(text, url, NULL, 0, links, type);
}

static void
video_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	add_url_mark(text, url, NULL, 0, links, "video");
}

static void
source_handler(struct string *text, struct link_list *links, GumboVector *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	add_url_mark(text, url, NULL, 0, links, type);
}

static const struct html_element_handler handlers[] = {
	{GUMBO_TAG_SPAN,     NULL,                  NULL},
	{GUMBO_TAG_A,        NULL,                  &a_handler},
	{GUMBO_TAG_SUP,      &sup_handler,          NULL},
	{GUMBO_TAG_IMG,      &img_handler,          NULL},
	{GUMBO_TAG_IFRAME,   &iframe_handler,       NULL},
	{GUMBO_TAG_EMBED,    &embed_handler,        NULL},
	{GUMBO_TAG_SOURCE,   &source_handler,       NULL},
	{GUMBO_TAG_Q,        &q_handler,            &q_handler},
	{GUMBO_TAG_CODE,     NULL,                  NULL},
	{GUMBO_TAG_I,        NULL,                  NULL},
	{GUMBO_TAG_B,        NULL,                  NULL},
	{GUMBO_TAG_U,        NULL,                  NULL},
	{GUMBO_TAG_EM,       NULL,                  NULL},
	{GUMBO_TAG_MARK,     NULL,                  NULL},
	{GUMBO_TAG_SMALL,    NULL,                  NULL},
	{GUMBO_TAG_TIME,     NULL,                  NULL},
	{GUMBO_TAG_STRONG,   NULL,                  NULL},
	{GUMBO_TAG_VIDEO,    &video_handler,        NULL},
	{GUMBO_TAG_LABEL,    NULL,                  NULL},
	{GUMBO_TAG_TEXTAREA, NULL,                  NULL},
	{GUMBO_TAG_THEAD,    NULL,                  NULL},
	{GUMBO_TAG_TBODY,    NULL,                  NULL},
	{GUMBO_TAG_TFOOT,    NULL,                  NULL},
	{GUMBO_TAG_NOSCRIPT, NULL,                  NULL},
	{GUMBO_TAG_BUTTON,   &button_start_handler, &button_end_handler},
	// These are ignored by parsing function.
	//{GUMBO_TAG_STYLE,  NULL,                  NULL},
	//{GUMBO_TAG_SCRIPT, NULL,                  NULL},
	{GUMBO_TAG_UNKNOWN,  NULL,                  NULL},
};

static void
dump_html(GumboNode *node, struct string *text, struct link_list *links)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i;
		for (i = 0; handlers[i].tag_id != GUMBO_TAG_UNKNOWN; ++i) {
			if (node->v.element.tag == handlers[i].tag_id) {
				break;
			}
		}
		if (handlers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			catas(text, node->v.element.original_tag.data, node->v.element.original_tag.length);
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], text, links);
			}
			catas(text, node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
		} else {
			if (handlers[i].start_handler != NULL) {
				handlers[i].start_handler(text, links, &node->v.element.attributes);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], text, links);
			}
			if (handlers[i].end_handler != NULL) {
				handlers[i].end_handler(text, links, &node->v.element.attributes);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT) || (node->type == GUMBO_NODE_CDATA)) {
		for (size_t j = 0; j < node->v.text.original_text.length; ++j) {
			if ((text->len > 0)
				&& (ISWHITESPACE(text->ptr[text->len - 1]))
				&& (ISWHITESPACE(node->v.text.original_text.data[j]))) {
				continue;
			}
			if (ISWHITESPACE(node->v.text.original_text.data[j])) {
				catcs(text, ' ');
			} else {
				catcs(text, node->v.text.original_text.data[j]);
			}
		}
	}
}

struct wstring *
prepare_to_render_text_html(const struct wstring *wide_src, struct link_list *links)
{
	struct string *text = crtes();
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
	dump_html(output->root, text, links);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_string(src);
	struct wstring *wtext = convert_string_to_wstring(text);
	free_string(text);
	return wtext;
}
