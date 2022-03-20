#include <string.h>
#include <tidybuffio.h>
#include "update_feed/parse_feed/parse_feed.h"

bool
we_are_inside_item(const struct xml_data *data)
{
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	if ((data->atom10_pos & ATOM10_ENTRY) != 0) {
		return true;
	}
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	if ((data->rss20_pos & RSS20_ITEM) != 0) {
		return true;
	}
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	if ((data->rss11_pos & RSS11_ITEM) != 0) {
		return true;
	}
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	if ((data->atom03_pos & ATOM03_ENTRY) != 0) {
		return true;
	}
#endif
	return false;
}

const char *
get_value_of_attribute_key(const TidyAttr attrs, const char *key)
{
	const char *attr_name;
	for (TidyAttr attr = attrs; attr != NULL; attr = tidyAttrNext(attr)) {
		attr_name = tidyAttrName(attr);
		if (attr_name == NULL) {
			continue;
		}
		if (strcmp(attr_name, key) == 0) {
			return tidyAttrValue(attr); // success
		}
	}
	return NULL; // failure, didn't find an attribute with key name
}

static inline const struct string *
get_namespace_uri(const struct xml_namespace_stack *namespaces, const char *namespace_name, size_t namespace_name_len)
{
	for (size_t i = 0; i < namespaces->top; ++i) {
		if ((namespace_name_len == namespaces->buf[i].name->len) &&
		    (memcmp(namespace_name, namespaces->buf[i].name->ptr, namespace_name_len) == 0))
		{
			INFO("Found URI of the \"%s\" namespace: \"%s\".", namespaces->buf[i].name->ptr, namespaces->buf[i].uri->ptr);
			return namespaces->buf[i].uri;
		}
	}
	return NULL;
}

static inline bool
find_namespaces_among_tag_attributes_and_process_them(struct xml_data *data, const TidyAttr attrs, bool *changed_default_namespace)
{
	const char *attr_name;
	const char *namespace_name;
	const char *namespace_uri;
	for (TidyAttr attr = attrs; attr != NULL; attr = tidyAttrNext(attr)) {
		attr_name = tidyAttrName(attr);
		if (attr_name == NULL) {
			continue;
		}
		if (strstr(attr_name, "xmlns") != attr_name) {
			// Attribute name doesn't start with "xmlns".
			continue;
		}
		if (*(attr_name + 5) == ':') { // Attribute name starts with "xmlns:"
			if ((namespace_uri = tidyAttrValue(attr)) == NULL) {
				continue;
			}
			namespace_name = attr_name + 6;
			if (add_namespace_to_stack(&data->namespaces, namespace_name, namespace_uri) == false) {
				return false;
			}
		} else if (*(attr_name + 5) == '\0') { // Attribute name is just "xmlns".
			if ((namespace_uri = tidyAttrValue(attr)) == NULL) {
				continue;
			}
			if (*changed_default_namespace == true) {
				// Don't add another default namespace because XML tag must not have more than one "xmlns" attribute set.
				// However, we give the last "xmlns" attribute a higher priority here.
				if (cpyas(data->def_ns->uri, namespace_uri, strlen(namespace_uri)) == false) {
					return false;
				}
			} else {
				if (prepend_default_namespace(&data->def_ns, namespace_uri, strlen(namespace_uri)) == false) {
					return false;
				}
				*changed_default_namespace = true;
			}
		}
	}
	return true;
}

static inline void
stuff_to_do_when_xml_element_starts(struct xml_data *data, const struct string *namespace, const char *name, const TidyAttr attrs, bool has_prefix)
{
	++(data->depth);
	empty_string(data->value);
	if (namespace != NULL) {
		parse_namespace_element_start(data, namespace, name, attrs);
		return;
	} else if ((has_prefix == false) && (data->start_handler != NULL)) {
		data->start_handler(data, name, attrs);
		return;
	}
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	if ((data->depth == 1) && (strcmp(name, "rss") == 0)) {
		const char *version = get_value_of_attribute_key(attrs, "version");
		if ((version != NULL) &&
			((strcmp(version, "2.0") == 0) ||
			(strcmp(version, "0.94") == 0) ||
			(strcmp(version, "0.93") == 0) ||
			(strcmp(version, "0.92") == 0) ||
			(strcmp(version, "0.91") == 0)))
		{
			data->start_handler = &parse_rss20_element_start;
			data->end_handler = &parse_rss20_element_end;
		}
	}
#endif
}

static inline void
stuff_to_do_when_xml_element_ends(struct xml_data *data, const struct string *tag_namespace, const char *tag_name, bool has_prefix)
{
	(data->depth)--;
	trim_whitespace_from_string(data->value);

	if (tag_namespace != NULL) {
		parse_namespace_element_end(data, tag_namespace, tag_name);
	} else if ((has_prefix == false) && (data->end_handler != NULL)) {
		data->end_handler(data, tag_name);
	}
}

static bool
dumpNode(TidyDoc *tdoc, TidyNode tnod, TidyBuffer *buf, struct xml_data *data)
{
	uint16_t old_namespaces_count;
	bool changed_default_namespace;
	const char *tag_name;               // Name of the tag.
	const struct string *tag_namespace; // Namespace URI to which the tag belongs.
	const char *sep_pos;                // Pointer to the character that separates name and prefix of the tag.

	TidyNodeType child_type;
	TidyAttr attrs;

	for (TidyNode child = tidyGetChild(tnod); child != NULL; child = tidyGetNext(child)) {
		child_type = tidyNodeGetType(child);
		if ((child_type == TidyNode_Text) || (child_type == TidyNode_CDATA)) {

			tidyBufClear(buf);
			tidyNodeGetValue(*tdoc, child, buf);
			if (buf->bp != NULL) {
				catas(data->value, (char *)buf->bp, strlen((char *)buf->bp));
			}

		} else if ((child_type == TidyNode_Start) || (child_type == TidyNode_StartEnd)) {

			attrs = tidyAttrFirst(child);
			changed_default_namespace = false;
			old_namespaces_count = data->namespaces.top;
			if (find_namespaces_among_tag_attributes_and_process_them(data, attrs, &changed_default_namespace) == false) {
				return false;
			}

			tag_name = tidyNodeGetName(child);
			tag_namespace = NULL;
			sep_pos = strchr(tag_name, XML_NAMESPACE_SEPARATOR);
			if (sep_pos != NULL) {
				tag_namespace = get_namespace_uri(&data->namespaces, tag_name, sep_pos - tag_name);
				tag_name = sep_pos + 1;
			} else if (data->def_ns != NULL) {
				tag_namespace = data->def_ns->uri;
			}

			stuff_to_do_when_xml_element_starts(data, tag_namespace, tag_name, attrs, sep_pos == NULL ? false : true);

			if (dumpNode(tdoc, child, buf, data) == false) {
				return false;
			}

			stuff_to_do_when_xml_element_ends(data, tag_namespace, tag_name, sep_pos == NULL ? false : true);

			if (changed_default_namespace == true) {
				discard_default_namespace(&data->def_ns);
			}
			while (data->namespaces.top != old_namespaces_count) {
				pop_namespace_from_stack(&data->namespaces);
			}

		}
	}

	return true;
}

static bool
enter_xml_parsing_loop(const struct string *feed_buf, struct xml_data *data)
{
	TidyDoc tdoc = tidyCreate();
	TidyBuffer draft_buffer;
	TidyBuffer error_buffer;
	tidyBufInit(&draft_buffer);
	tidyBufInit(&error_buffer);
	tidySetErrorBuffer(tdoc, &error_buffer);
	tidyOptSetBool(tdoc, TidyXmlTags, true); // Enable XML mode.
	tidyParseString(tdoc, feed_buf->ptr);
	tidyCleanAndRepair(tdoc);
	tidyRunDiagnostics(tdoc);

	if (error_buffer.bp != NULL) {
		INFO("Tidy's report:\n%s", error_buffer.bp);
	} else {
		INFO("Tidy's error buffer is nullified.");
	}

	bool success = true;

	if (dumpNode(&tdoc, tidyGetRoot(tdoc), &draft_buffer, data) == false) {
		FAIL("Something really bad happened during XML parsing!");
		success = false;
	}

	tidyBufFree(&error_buffer);
	tidyBufFree(&draft_buffer);
	tidyRelease(tdoc);

	return success;
}

struct getfeed_feed *
parse_xml_feed(const struct string *feed_buf)
{
	struct xml_data data;
	if ((data.value = crtes()) == NULL) {
		FAIL("Not enough memory for updating a feed!");
		return NULL;
	}
	if ((data.feed = create_feed()) == NULL) {
		FAIL("Not enough memory for updating a feed!");
		free_string(data.value);
		return NULL;
	}
	data.def_ns = NULL;
	data.namespaces.top = 0;
	data.namespaces.lim = 0;
	data.namespaces.buf = NULL;
	data.depth = 0;
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	data.rss20_pos = RSS20_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	data.atom10_pos = ATOM10_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	data.atom03_pos = ATOM03_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	data.dc_pos = DC_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
	data.yandex_pos = YANDEX_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	data.rss11_pos = RSS11_NONE;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	data.rss10content_pos = RSS10CONTENT_NONE;
#endif
	data.start_handler = NULL;
	data.end_handler = NULL;
	data.error = PARSE_OKAY;

	if (enter_xml_parsing_loop(feed_buf, &data) == false) {
		FAIL("Feed update stopped due to parse error!");
		free_string(data.value);
		free_feed(data.feed);
		return NULL;
	}

	free_default_namespaces(data.def_ns);
	free_namespace_stack(&data.namespaces);
	free_string(data.value);

	return data.feed;
}
