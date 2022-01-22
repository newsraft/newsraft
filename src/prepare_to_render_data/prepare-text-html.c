#include <tidy.h>
#include <tidybuffio.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

enum html_position {
	HTML_NONE = 0,
	HTML_STYLE = 1,
	HTML_SCRIPT = 2,
	HTML_VIDEO = 4,
};

static const char *
get_value_of_xml_attribute(const TidyAttr *attrs, const char *attr_name)
{
	for (TidyAttr attr = *attrs; attr; attr = tidyAttrNext(attr)) {
		if (strcmp(attr_name, tidyAttrName(attr)) == 0) {
			return tidyAttrValue(attr);
		}
	}
	return NULL;
}

static inline void
sup_start_handler(struct string *text)
{
	catcs(text, '^');
}

static inline void
q_handler(struct string *text)
{
	catcs(text, '"');
}

static inline void
button_start_handler(struct string *text)
{
	catcs(text, '[');
}

static inline void
button_end_handler(struct string *text)
{
	catcs(text, ']');
}

static inline void
script_start_handler(enum html_position *pos)
{
	*pos |= HTML_SCRIPT;
}

static inline void
script_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_SCRIPT;
}

static inline void
style_start_handler(enum html_position *pos)
{
	*pos |= HTML_STYLE;
}

static inline void
style_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_STYLE;
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

	// Add link mark to HTML content.
	char *url_mark;
	size_t url_mark_len = 0;
	if (data_type == NULL) {
		if ((title != NULL) && (title_len != 0)) {
			url_mark_len = snprintf(NULL, 0, " [%zu, \"%s\"]", links->len + 1, title);
			if ((url_mark = malloc(sizeof(char) * (url_mark_len + 1))) == NULL) {
				return;
			}
			snprintf(url_mark, url_mark_len + 1, " [%zu, \"%s\"]", links->len + 1, title);
		} else {
			url_mark_len = snprintf(NULL, 0, " [%zu]", links->len + 1);
			if ((url_mark = malloc(sizeof(char) * (url_mark_len + 1))) == NULL) {
				return;
			}
			snprintf(url_mark, url_mark_len + 1, " [%zu]", links->len + 1);
		}
	} else {
		if ((title != NULL) && (title_len != 0)) {
			url_mark_len = snprintf(NULL, 0, " [%zu, %s \"%s\"]", links->len + 1, data_type, title);
			if ((url_mark = malloc(sizeof(char) * (url_mark_len + 1))) == NULL) {
				return;
			}
			snprintf(url_mark, url_mark_len + 1, " [%zu, %s \"%s\"]", links->len + 1, data_type, title);
		} else {
			url_mark_len = snprintf(NULL, 0, " [%zu, %s]", links->len + 1, data_type);
			if ((url_mark = malloc(sizeof(char) * (url_mark_len + 1))) == NULL) {
				return;
			}
			snprintf(url_mark, url_mark_len + 1, " [%zu, %s]", links->len + 1, data_type);
		}
	}
	catas(text, url_mark, url_mark_len);
	free(url_mark);

	// Add URL to link list.
	add_another_url_to_trim_link_list(links, url, url_len);
}

static inline void
a_end_handler(struct string *text, struct link_list *links, const TidyAttr *atts)
{
	const char *url = get_value_of_xml_attribute(atts, "href");
	const char *title = get_value_of_xml_attribute(atts, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	add_url_mark(text, url, title, title_len, links, NULL);
}

static inline void
img_start_handler(struct string *text, const TidyAttr *atts, struct link_list *links)
{
	const char *url = get_value_of_xml_attribute(atts, "src");
	const char *title = get_value_of_xml_attribute(atts, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	if ((title == NULL) || (title_len == 0)) {
		title = get_value_of_xml_attribute(atts, "alt");
		if (title != NULL) {
			title_len = strlen(title);
		}
	}
	add_url_mark(text, url, title, title_len, links, "image");
}

static inline void
iframe_start_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
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
	add_url_mark(text, url, title, title_len, links, "frame");
}

static inline void
embed_start_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	if (type == NULL) {
		type = "embed";
	}
	add_url_mark(text, url, NULL, 0, links, type);
}

static inline bool
start_handler(TidyTagId t, struct string *w, enum html_position *p, struct link_list *l, const TidyAttr *a)
{
	     if (t == TidyTag_SPAN)     { /* just nothing */             return true; }
	else if (t == TidyTag_SUP)      { sup_start_handler(w);          return true; }
	else if (t == TidyTag_A)        { /* just nothing */             return true; }
	else if (t == TidyTag_IMG)      { img_start_handler(w, a, l);    return true; }
	else if (t == TidyTag_IFRAME)   { iframe_start_handler(w, a, l); return true; }
	else if (t == TidyTag_EMBED)    { embed_start_handler(w, a, l);  return true; }
	else if (t == TidyTag_VIDEO)    { /* TODO */                     return true; }
	else if (t == TidyTag_SOURCE)   { /* TODO */                     return true; }
	else if (t == TidyTag_Q)        { q_handler(w);                  return true; }
	else if (t == TidyTag_CODE)     { /* TODO */                     return true; }
	else if (t == TidyTag_B)        { /* TODO */                     return true; }
	else if (t == TidyTag_I)        { /* TODO */                     return true; }
	else if (t == TidyTag_EM)       { /* TODO */                     return true; }
	else if (t == TidyTag_MARK)     { /* TODO */                     return true; }
	else if (t == TidyTag_SMALL)    { /* TODO */                     return true; }
	else if (t == TidyTag_TIME)     { /* TODO */                     return true; }
	else if (t == TidyTag_STRONG)   { /* TODO */                     return true; }
	else if (t == TidyTag_LABEL)    { /* just nothing */             return true; }
	else if (t == TidyTag_TEXTAREA) { /* just nothing */             return true; }
	else if (t == TidyTag_THEAD)    { /* just nothing */             return true; }
	else if (t == TidyTag_TBODY)    { /* just nothing */             return true; }
	else if (t == TidyTag_TFOOT)    { /* just nothing */             return true; }
	else if (t == TidyTag_NOSCRIPT) { /* just nothing */             return true; }
	else if (t == TidyTag_BUTTON)   { button_start_handler(w);       return true; }
	else if (t == TidyTag_SCRIPT)   { script_start_handler(p);       return true; }
	else if (t == TidyTag_STYLE)    { style_start_handler(p);        return true; }

	return false;
}

static inline bool
end_handler(TidyTagId t, struct string *w, enum html_position *p, struct link_list *l, const TidyAttr *a)
{
	     if (t == TidyTag_SPAN)     { /* just nothing */      return true; }
	else if (t == TidyTag_SUP)      { /* just nothing */      return true; }
	else if (t == TidyTag_A)        { a_end_handler(w, l, a); return true; }
	else if (t == TidyTag_VIDEO)    { /* TODO */              return true; }
	else if (t == TidyTag_Q)        { q_handler(w);           return true; }
	else if (t == TidyTag_CODE)     { /* TODO */              return true; }
	else if (t == TidyTag_B)        { /* TODO */              return true; }
	else if (t == TidyTag_I)        { /* TODO */              return true; }
	else if (t == TidyTag_EM)       { /* TODO */              return true; }
	else if (t == TidyTag_MARK)     { /* TODO */              return true; }
	else if (t == TidyTag_SMALL)    { /* TODO */              return true; }
	else if (t == TidyTag_TIME)     { /* TODO */              return true; }
	else if (t == TidyTag_STRONG)   { /* TODO */              return true; }
	else if (t == TidyTag_LABEL)    { /* just nothing */      return true; }
	else if (t == TidyTag_TEXTAREA) { /* just nothing */      return true; }
	else if (t == TidyTag_THEAD)    { /* just nothing */      return true; }
	else if (t == TidyTag_TBODY)    { /* just nothing */      return true; }
	else if (t == TidyTag_TFOOT)    { /* just nothing */      return true; }
	else if (t == TidyTag_NOSCRIPT) { /* just nothing */      return true; }
	else if (t == TidyTag_BUTTON)   { button_end_handler(w);  return true; }
	else if (t == TidyTag_SCRIPT)   { script_end_handler(p);  return true; }
	else if (t == TidyTag_STYLE)    { style_end_handler(p);   return true; }
	// These elements are self-closing, but some generators tend to append
	// redundant closing tag. Don't return any errors here.
	else if (t == TidyTag_IMG)      { return true; }
	else if (t == TidyTag_IFRAME)   { return true; }
	else if (t == TidyTag_SOURCE)   { return true; }
	else if (t == TidyTag_EMBED)    { return true; }

	return false;
}

static void
cat_tag_to_string(struct string *target, const char *tag_name, const TidyAttr *atts, bool is_start)
{
	if (tag_name == NULL) {
		return;
	}
	size_t tag_name_len = strlen(tag_name);
	if (tag_name_len == 0) {
		return;
	}

	catcs(target, '<');
	if (is_start == false) {
		catcs(target, '/');
	}
	catas(target, tag_name, tag_name_len);

	// Add tag attributes.
	const char *attr_name;
	size_t attr_name_len;
	const char *attr_value;
	size_t attr_value_len;
	for (TidyAttr attr = *atts; attr; attr = tidyAttrNext(attr)) {
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

static void
dumpNode(TidyDoc *tdoc, TidyNode tnod, TidyBuffer *buf, struct string *text, enum html_position *pos, struct link_list *links)
{
	const char *child_name;
	TidyTagId child_id;
	TidyNodeType child_type;
	TidyAttr child_atts;
	for (TidyNode child = tidyGetChild(tnod); child; child = tidyGetNext(child)) {
		child_type = tidyNodeGetType(child);
		if ((child_type == TidyNode_Text) || (child_type == TidyNode_CDATA)) {
			tidyBufClear(buf);
			tidyNodeGetValue(*tdoc, child, buf);
			if (buf->bp != NULL) {
				catas(text, (char *)buf->bp, strlen((char *)buf->bp));
			}
		} else if ((child_type == TidyNode_Start) || (child_type == TidyNode_StartEnd)) {
			child_name = tidyNodeGetName(child);
			child_id = tidyNodeGetId(child);
			child_atts = tidyAttrFirst(child);
			if (start_handler(child_id, text, pos, links, &child_atts) == false) {
				cat_tag_to_string(text, child_name, &child_atts, true);
			}
			dumpNode(tdoc, child, buf, text, pos, links);
			if (end_handler(child_id, text, pos, links, &child_atts) == false) {
				cat_tag_to_string(text, child_name, &child_atts, false);
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
	enum html_position html_pos = HTML_NONE;

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

	dumpNode(&tdoc, tidyGetBody(tdoc), &tempbuf, text, &html_pos, links);

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
