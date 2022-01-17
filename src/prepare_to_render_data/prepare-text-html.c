#include "prepare_to_render_data/prepare_to_render_data.h"

enum html_position {
	HTML_NONE = 0,
	HTML_STYLE = 1,
	HTML_SCRIPT = 2,
	HTML_VIDEO = 4,
};

static struct wstring *anchor_url = NULL;

static inline void
sup_start_handler(struct wstring *text)
{
	wcatcs(text, L'^');
}

static inline void
q_start_handler(struct wstring *text)
{
	wcatcs(text, L'"');
}

static inline void
button_start_handler(struct wstring *text)
{
	wcatcs(text, L'{');
}

static inline void
button_end_handler(struct wstring *text)
{
	wcatcs(text, L'}');
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
add_url_mark(struct wstring *text, const struct wstring *url, const struct wstring *title, struct link_list *links, const wchar_t *url_type_wstr)
{
	if ((url == NULL) || (url->len == 0)) {
		return;
	}
	struct string *str = convert_wstring_to_string(url);
	if (str == NULL) {
		return;
	}

	wchar_t url_index[10000];
	size_t url_index_len = 0;

	if (url_type_wstr == NULL) {
		if ((title != NULL) && (title->len != 0)) {
			url_index_len = swprintf(url_index, 10000, L" [%zu, \"%ls\"]", links->len + 1, title->ptr);
		} else {
			url_index_len = swprintf(url_index, 10000, L" [%zu]", links->len + 1);
		}
	} else {
		if ((title != NULL) && (title->len != 0)) {
			url_index_len = swprintf(url_index, 10000, L" [%zu, %ls \"%ls\"]", links->len + 1, url_type_wstr, title->ptr);
		} else {
			url_index_len = swprintf(url_index, 10000, L" [%zu, %ls]", links->len + 1, url_type_wstr);
		}
	}

	wcatas(text, url_index, url_index_len);
	add_another_url_to_trim_link_list(links, str->ptr, str->len);
	free_string(str);
}

static inline void
a_start_handler(struct xml_tag *tag)
{
	const struct wstring *url = get_value_of_xml_attribute(tag, L"href");
	if (url != NULL) {
		wcpyss(anchor_url, url);
	}
}

static inline void
a_end_handler(struct wstring *text, struct link_list *links)
{
	add_url_mark(text, anchor_url, NULL, links, NULL);
}

static inline void
img_start_handler(struct wstring *text, struct xml_tag *tag, struct link_list *links)
{
	const struct wstring *url = get_value_of_xml_attribute(tag, L"src");
	const struct wstring *title = get_value_of_xml_attribute(tag, L"title");
	if ((title == NULL) || (title->len == 0)) {
		title = get_value_of_xml_attribute(tag, L"alt");
	}
	add_url_mark(text, url, title, links, L"image");
}

static inline void
iframe_start_handler(struct wstring *text, struct xml_tag *tag, struct link_list *links)
{
	const struct wstring *url = get_value_of_xml_attribute(tag, L"src");
	const struct wstring *title = get_value_of_xml_attribute(tag, L"title");
	if ((title == NULL) || (title->len == 0)) {
		title = get_value_of_xml_attribute(tag, L"name");
	}
	add_url_mark(text, url, title, links, L"frame");
}

static inline bool
start_handler(wchar_t *t, struct wstring *w, enum html_position *p, struct xml_tag *tag, struct link_list *l)
{
	     if (wcscmp(t, L"span")     == 0) { /* just nothing */               return true; }
	else if (wcscmp(t, L"sup")      == 0) { sup_start_handler(w);            return true; }
	else if (wcscmp(t, L"a")        == 0) { a_start_handler(tag);            return true; }
	else if (wcscmp(t, L"img")      == 0) { img_start_handler(w, tag, l);    return true; }
	else if (wcscmp(t, L"iframe")   == 0) { iframe_start_handler(w, tag, l); return true; }
	else if (wcscmp(t, L"video")    == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"source")   == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"q")        == 0) { q_start_handler(w);              return true; }
	else if (wcscmp(t, L"code")     == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"b")        == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"i")        == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"em")       == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"mark")     == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"small")    == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"time")     == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"strong")   == 0) { /* TODO */                       return true; }
	else if (wcscmp(t, L"label")    == 0) { /* just nothing */               return true; }
	else if (wcscmp(t, L"textarea") == 0) { /* just nothing */               return true; }
	else if (wcscmp(t, L"button")   == 0) { button_start_handler(w);         return true; }
	else if (wcscmp(t, L"script")   == 0) { script_start_handler(p);         return true; }
	else if (wcscmp(t, L"style")    == 0) { style_start_handler(p);          return true; }

	return false;
}

static inline bool
end_handler(wchar_t *t, struct wstring *w, enum html_position *p, struct link_list *l)
{
	     if (wcscmp(t, L"span")     == 0) { /* just nothing */     return true; }
	else if (wcscmp(t, L"sup")      == 0) { /* just nothing */     return true; }
	else if (wcscmp(t, L"a")        == 0) { a_end_handler(w, l);   return true; }
	else if (wcscmp(t, L"video")    == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"q")        == 0) { q_start_handler(w);    return true; }
	else if (wcscmp(t, L"code")     == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"b")        == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"i")        == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"em")       == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"mark")     == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"small")    == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"time")     == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"strong")   == 0) { /* TODO */             return true; }
	else if (wcscmp(t, L"label")    == 0) { /* just nothing */     return true; }
	else if (wcscmp(t, L"textarea") == 0) { /* just nothing */     return true; }
	else if (wcscmp(t, L"button")   == 0) { button_end_handler(w); return true; }
	else if (wcscmp(t, L"script")   == 0) { script_end_handler(p); return true; }
	else if (wcscmp(t, L"style")    == 0) { style_end_handler(p);  return true; }
	// These elements are self-closing, but some generators tend to append
	// redundant closing tag. Don't return any errors here.
	else if (wcscmp(t, L"img")      == 0) { return true; }
	else if (wcscmp(t, L"iframe")   == 0) { return true; }
	else if (wcscmp(t, L"source")   == 0) { return true; }

	return false;
}

struct wstring *
prepare_to_render_text_html(const struct wstring *wstr, struct link_list *links)
{
	struct wstring *t = wcrtes();
	if (t == NULL) {
		FAIL("Not enough memory for text buffer to prepare HTML render block!");
		return NULL;
	}
	struct xml_tag *tag = create_tag();
	if (tag == NULL) {
		FAIL("Not enough memory for tag structure to prepare HTML render block!");
		free_wstring(t);
		return NULL;
	}
	anchor_url = wcrtes();
	if (anchor_url == NULL) {
		FAIL("Not enough memory for anchor buffer to prepare HTML render block!");
		free_tag(tag);
		free_wstring(t);
		return NULL;
	}
	bool in_tag = false;
	bool found_tag;
	const wchar_t *i = wstr->ptr;
	enum html_position html_pos = HTML_NONE;
	while (*i != L'\0') {
		if (in_tag == false) {
			if (*i == L'<') {
				in_tag = true;
				empty_tag(tag);
			} else {
				// Avoid characters of certain elements.
				if ((html_pos & (HTML_STYLE|HTML_SCRIPT|HTML_VIDEO)) == 0) {
					if (wcatcs(t, *i) == false) { goto error; }
				}
			}
		} else {
			if (append_wchar_to_tag(tag, *i) == XML_TAG_DONE) {
				in_tag = false;
				found_tag = false;
				if (tag->atts[0].name->ptr[0] == L'/') {
					found_tag = end_handler(tag->atts[0].name->ptr + 1, t, &html_pos, links);
				} else {
					found_tag = start_handler(tag->atts[0].name->ptr, t, &html_pos, tag, links);
				}
				if (found_tag == false) {
					if (wcatcs(t, L'<')     == false) { goto error; }
					if (wcatss(t, tag->buf) == false) { goto error; }
					if (wcatcs(t, L'>')     == false) { goto error; }
				}
			}
		}
		++i;
	}
	free_wstring(anchor_url);
	free_tag(tag);
	return t;
error:
	free_wstring(t);
	free_tag(tag);
	free_wstring(anchor_url);
	return NULL;
}
