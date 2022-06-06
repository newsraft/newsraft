#include <tidy.h>
#include <tidybuffio.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

struct html_element_handler {
	const TidyTagId tag_id;
	void (*start_handler)(struct string *, struct link_list *, const TidyAttr *);
	void (*end_handler)(struct string *, struct link_list *, const TidyAttr *);
};

static const char *
get_value_of_xml_attribute(const TidyAttr *attrs, const char *attr_name)
{
	for (TidyAttr attr = *attrs; attr != NULL; attr = tidyAttrNext(attr)) {
		if (strcmp(attr_name, tidyAttrName(attr)) == 0) {
			return tidyAttrValue(attr);
		}
	}
	return NULL;
}

static void
sup_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '^');
}

static void
q_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '"');
}

static void
button_start_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	(void)links;
	(void)attrs;
	catcs(text, '[');
}

static void
button_end_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
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

	int64_t url_index = add_another_url_to_trim_link_list(links, url, url_len);
	if (url_index == -1) {
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
a_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
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
img_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
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
iframe_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
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
embed_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	if (type == NULL) {
		type = "embed";
	}
	add_url_mark(text, url, NULL, 0, links, type);
}

static void
video_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	add_url_mark(text, url, NULL, 0, links, "video");
}

static void
source_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	add_url_mark(text, url, NULL, 0, links, type);
}

static const struct html_element_handler handlers[] = {
	{TidyTag_SPAN,     NULL,                  NULL},
	{TidyTag_A,        NULL,                  &a_handler},
	{TidyTag_SUP,      &sup_handler,          NULL},
	{TidyTag_IMG,      &img_handler,          NULL},
	{TidyTag_IFRAME,   &iframe_handler,       NULL},
	{TidyTag_EMBED,    &embed_handler,        NULL},
	{TidyTag_SOURCE,   &source_handler,       NULL},
	{TidyTag_Q,        &q_handler,            &q_handler},
	{TidyTag_CODE,     NULL,                  NULL},
	{TidyTag_B,        NULL,                  NULL},
	{TidyTag_I,        NULL,                  NULL},
	{TidyTag_EM,       NULL,                  NULL},
	{TidyTag_MARK,     NULL,                  NULL},
	{TidyTag_SMALL,    NULL,                  NULL},
	{TidyTag_TIME,     NULL,                  NULL},
	{TidyTag_STRONG,   NULL,                  NULL},
	{TidyTag_VIDEO,    &video_handler,        NULL},
	{TidyTag_LABEL,    NULL,                  NULL},
	{TidyTag_TEXTAREA, NULL,                  NULL},
	{TidyTag_THEAD,    NULL,                  NULL},
	{TidyTag_TBODY,    NULL,                  NULL},
	{TidyTag_TFOOT,    NULL,                  NULL},
	{TidyTag_NOSCRIPT, NULL,                  NULL},
	{TidyTag_BUTTON,   &button_start_handler, &button_end_handler},
	// These are ignored by parsing function.
	//{TidyTag_STYLE,  NULL,                  NULL},
	//{TidyTag_SCRIPT, NULL,                  NULL},
	{TidyTag_UNKNOWN,  NULL,                  NULL},
};

static inline void
cat_opening_tag_to_string(struct string *target, const char *tag_name, const TidyAttr *attrs)
{
	if (tag_name == NULL) {
		return;
	}
	size_t tag_name_len = strlen(tag_name);
	if (tag_name_len == 0) {
		return;
	}

	catcs(target, '<');
	catas(target, tag_name, tag_name_len);

	// Add tag attributes.
	const char *attr_name;
	size_t attr_name_len;
	const char *attr_value;
	size_t attr_value_len;
	for (TidyAttr attr = *attrs; attr != NULL; attr = tidyAttrNext(attr)) {
		attr_name = tidyAttrName(attr);
		if (attr_name == NULL) {
			continue;
		}
		attr_name_len = strlen(attr_name);
		if (attr_name_len == 0) {
			continue;
		}
		catcs(target, ' ');
		catas(target, attr_name, attr_name_len);
		attr_value = tidyAttrValue(attr);
		if (attr_value == NULL) {
			continue;
		}
		attr_value_len = strlen(attr_value);
		if (attr_value_len == 0) {
			continue;
		}
		if (strchr(attr_value, '"') == NULL) {
			catas(target, "=\"", 2);
			catas(target, attr_value, attr_value_len);
			catcs(target, '"');
		} else if (strchr(attr_value, '\'') == NULL) {
			catas(target, "='", 2);
			catas(target, attr_value, attr_value_len);
			catcs(target, '\'');
		} else {
			catas(target, "=ERROR", 6);
		}
	}

	catcs(target, '>');
}

static inline void
cat_closing_tag_to_string(struct string *target, const char *tag_name)
{
	if (tag_name == NULL) {
		return;
	}
	size_t tag_name_len = strlen(tag_name);
	if (tag_name_len == 0) {
		return;
	}
	catas(target, "</", 2);
	catas(target, tag_name, tag_name_len);
	catcs(target, '>');
}

static void
dumpNode(TidyDoc *tdoc, TidyNode tnod, TidyBuffer *buf, struct string *text, struct link_list *links)
{
	const char *child_name;
	TidyTagId child_id;
	TidyNodeType child_type;
	TidyAttr child_attrs;
	size_t i;
	for (TidyNode child = tidyGetChild(tnod); child != NULL; child = tidyGetNext(child)) {
		child_type = tidyNodeGetType(child);
		if ((child_type == TidyNode_Text) || (child_type == TidyNode_CDATA)) {
			tidyBufClear(buf);
			tidyNodeGetValue(*tdoc, child, buf);
			if (buf->bp != NULL) {
				catas(text, (char *)buf->bp, buf->size);
			}
		} else if ((child_type == TidyNode_Start) || (child_type == TidyNode_StartEnd)) {
			child_id = tidyNodeGetId(child);
			if ((child_id == TidyTag_STYLE) || (child_id == TidyTag_SCRIPT)) {
				// Completely ignore <style> and <script> elements.
				continue;
			}
			for (i = 0; handlers[i].tag_id != TidyTag_UNKNOWN; ++i) {
				if (child_id == handlers[i].tag_id) {
					break;
				}
			}
			child_attrs = tidyAttrFirst(child);
			if (handlers[i].tag_id == TidyTag_UNKNOWN) {
				child_name = tidyNodeGetName(child);
				cat_opening_tag_to_string(text, child_name, &child_attrs);
				dumpNode(tdoc, child, buf, text, links);
				cat_closing_tag_to_string(text, child_name);
			} else {
				if (handlers[i].start_handler != NULL) {
					handlers[i].start_handler(text, links, &child_attrs);
				}
				dumpNode(tdoc, child, buf, text, links);
				if (handlers[i].end_handler != NULL) {
					handlers[i].end_handler(text, links, &child_attrs);
				}
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

	TidyDoc tdoc = tidyCreate();
	TidyBuffer tempbuf = {0};
	TidyBuffer tidy_errbuf = {0};

	tidyBufInit(&tempbuf);

	tidySetErrorBuffer(tdoc, &tidy_errbuf);
	tidyOptSetInt(tdoc, TidyWrapLen, 0); // disable wrapping
	tidyOptSetBool(tdoc, TidyMakeBare, true); // use plain quotes instead fancy ones
	tidyOptSetBool(tdoc, TidyOmitOptionalTags, false);
	tidyOptSetBool(tdoc, TidyCoerceEndTags, true);

	// Don't convert entities to characters.
	tidyOptSetBool(tdoc, TidyAsciiChars, false);
	tidyOptSetBool(tdoc, TidyQuoteNbsp, true);
	tidyOptSetBool(tdoc, TidyQuoteMarks, true);
	tidyOptSetBool(tdoc, TidyQuoteAmpersand, true);
	tidyOptSetBool(tdoc, TidyPreserveEntities, true);

	tidyParseString(tdoc, src->ptr);
	tidyCleanAndRepair(tdoc);
	tidyRunDiagnostics(tdoc);

	dumpNode(&tdoc, tidyGetBody(tdoc), &tempbuf, text, links);

	if (tidy_errbuf.bp != NULL) {
		INFO("Tidy report:\n%s", tidy_errbuf.bp);
	} else {
		INFO("Tidy run silently.");
	}

	tidyBufFree(&tempbuf);
	tidyBufFree(&tidy_errbuf);
	tidyRelease(tdoc);
	free_string(src);
	struct wstring *wtext = convert_string_to_wstring(text);
	free_string(text);
	return wtext;
}
