#include <tidy.h>
#include <tidybuffio.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

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
sup_handler(struct string *text)
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
	struct string *url_mark = crtes();
	if (url_mark == NULL) {
		return;
	}
	if (data_type == NULL) {
		if ((title != NULL) && (title_len != 0)) {
			string_printf(url_mark, " [%zu, \"%s\"]", links->len + 1, title);
		} else {
			string_printf(url_mark, " [%zu]", links->len + 1);
		}
	} else {
		if ((title != NULL) && (title_len != 0)) {
			string_printf(url_mark, " [%zu, %s \"%s\"]", links->len + 1, data_type, title);
		} else {
			string_printf(url_mark, " [%zu, %s]", links->len + 1, data_type);
		}
	}
	catss(text, url_mark);
	free_string(url_mark);

	// Add URL to link list.
	add_another_url_to_trim_link_list(links, url, url_len);
}

static inline void
a_handler(struct string *text, struct link_list *links, const TidyAttr *attrs)
{
	const char *url = get_value_of_xml_attribute(attrs, "href");
	const char *title = get_value_of_xml_attribute(attrs, "title");
	size_t title_len;
	if (title != NULL) {
		title_len = strlen(title);
	}
	add_url_mark(text, url, title, title_len, links, NULL);
}

static inline void
img_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
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

static inline void
iframe_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
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
embed_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	if (type == NULL) {
		type = "embed";
	}
	add_url_mark(text, url, NULL, 0, links, type);
}

static inline void
video_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	add_url_mark(text, url, NULL, 0, links, "video");
}

static inline void
source_handler(struct string *text, const TidyAttr *attrs, struct link_list *links)
{
	const char *url = get_value_of_xml_attribute(attrs, "src");
	const char *type = get_value_of_xml_attribute(attrs, "type");
	add_url_mark(text, url, NULL, 0, links, type);
}

static inline bool
start_handler(TidyTagId t, struct string *w, struct link_list *l, const TidyAttr *a)
{
	     if (t == TidyTag_SPAN)     { /* just nothing */       return true; }
	else if (t == TidyTag_A)        { /* just nothing */       return true; }
	else if (t == TidyTag_SUP)      { sup_handler(w);          return true; }
	else if (t == TidyTag_IMG)      { img_handler(w, a, l);    return true; }
	else if (t == TidyTag_IFRAME)   { iframe_handler(w, a, l); return true; }
	else if (t == TidyTag_EMBED)    { embed_handler(w, a, l);  return true; }
	else if (t == TidyTag_SOURCE)   { source_handler(w, a, l); return true; }
	else if (t == TidyTag_Q)        { q_handler(w);            return true; }
	else if (t == TidyTag_CODE)     { /* TODO */               return true; }
	else if (t == TidyTag_B)        { /* TODO */               return true; }
	else if (t == TidyTag_I)        { /* TODO */               return true; }
	else if (t == TidyTag_EM)       { /* TODO */               return true; }
	else if (t == TidyTag_MARK)     { /* TODO */               return true; }
	else if (t == TidyTag_SMALL)    { /* TODO */               return true; }
	else if (t == TidyTag_TIME)     { /* TODO */               return true; }
	else if (t == TidyTag_STRONG)   { /* TODO */               return true; }
	else if (t == TidyTag_VIDEO)    { video_handler(w, a, l);  return true; }
	else if (t == TidyTag_LABEL)    { /* just nothing */       return true; }
	else if (t == TidyTag_TEXTAREA) { /* just nothing */       return true; }
	else if (t == TidyTag_THEAD)    { /* just nothing */       return true; }
	else if (t == TidyTag_TBODY)    { /* just nothing */       return true; }
	else if (t == TidyTag_TFOOT)    { /* just nothing */       return true; }
	else if (t == TidyTag_NOSCRIPT) { /* just nothing */       return true; }
	else if (t == TidyTag_BUTTON)   { button_start_handler(w); return true; }
	else if (t == TidyTag_SCRIPT)   { /* just nothing */       return true; }
	else if (t == TidyTag_STYLE)    { /* just nothing */       return true; }

	return false;
}

static inline bool
end_handler(TidyTagId t, struct string *w, struct link_list *l, const TidyAttr *a)
{
	     if (t == TidyTag_SPAN)     { /* just nothing */     return true; }
	else if (t == TidyTag_A)        { a_handler(w, l, a);    return true; }
	else if (t == TidyTag_SUP)      { /* just nothing */     return true; }
	else if (t == TidyTag_Q)        { q_handler(w);          return true; }
	else if (t == TidyTag_CODE)     { /* TODO */             return true; }
	else if (t == TidyTag_B)        { /* TODO */             return true; }
	else if (t == TidyTag_I)        { /* TODO */             return true; }
	else if (t == TidyTag_EM)       { /* TODO */             return true; }
	else if (t == TidyTag_MARK)     { /* TODO */             return true; }
	else if (t == TidyTag_SMALL)    { /* TODO */             return true; }
	else if (t == TidyTag_TIME)     { /* TODO */             return true; }
	else if (t == TidyTag_STRONG)   { /* TODO */             return true; }
	else if (t == TidyTag_VIDEO)    { /* just nothing */     return true; }
	else if (t == TidyTag_LABEL)    { /* just nothing */     return true; }
	else if (t == TidyTag_TEXTAREA) { /* just nothing */     return true; }
	else if (t == TidyTag_THEAD)    { /* just nothing */     return true; }
	else if (t == TidyTag_TBODY)    { /* just nothing */     return true; }
	else if (t == TidyTag_TFOOT)    { /* just nothing */     return true; }
	else if (t == TidyTag_NOSCRIPT) { /* just nothing */     return true; }
	else if (t == TidyTag_BUTTON)   { button_end_handler(w); return true; }
	else if (t == TidyTag_SCRIPT)   { /* just nothing */     return true; }
	else if (t == TidyTag_STYLE)    { /* just nothing */     return true; }
	// These elements are self-closing, but some generators tend to append
	// redundant closing tag. Don't return error here.
	else if (t == TidyTag_IMG)      { return true; }
	else if (t == TidyTag_IFRAME)   { return true; }
	else if (t == TidyTag_SOURCE)   { return true; }
	else if (t == TidyTag_EMBED)    { return true; }

	return false;
}

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
	for (TidyNode child = tidyGetChild(tnod); child != NULL; child = tidyGetNext(child)) {
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
			child_attrs = tidyAttrFirst(child);
			if (start_handler(child_id, text, links, &child_attrs) == false) {
				cat_opening_tag_to_string(text, child_name, &child_attrs);
			}
			// Don't descent into the <style> and <script> elements.
			if ((child_id != TidyTag_STYLE) && (child_id != TidyTag_SCRIPT)) {
				dumpNode(tdoc, child, buf, text, links);
			}
			if (end_handler(child_id, text, links, &child_attrs) == false) {
				cat_closing_tag_to_string(text, child_name);
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
