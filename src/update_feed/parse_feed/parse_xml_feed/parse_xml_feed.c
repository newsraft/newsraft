#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

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
		if (strncmp(attr_name, "xmlns", 5) != 0) {
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
	parse_element_start(data, namespace, name, attrs);
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	if ((data->depth == 1) && (namespace == NULL) && (strcmp(name, "rss") == 0)) {
		const char *version = get_value_of_attribute_key(attrs, "version");
		if ((version != NULL) &&
			((strcmp(version, "2.0") == 0) ||
			(strcmp(version, "0.94") == 0) ||
			(strcmp(version, "0.93") == 0) ||
			(strcmp(version, "0.92") == 0) ||
			(strcmp(version, "0.91") == 0)))
		{
			data->default_handler = RSS20_FORMAT;
		}
	}
#endif
}

static inline void
stuff_to_do_when_xml_element_ends(struct xml_data *data, const struct string *namespace, const char *name, const TidyAttr attrs, bool has_prefix)
{
	--(data->depth);
	trim_whitespace_from_string(data->value);
	parse_element_end(data, namespace, name, attrs);
}

static bool
dumpNode(struct xml_data *data, TidyNode tnod)
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

			tidyBufClear(&data->draft_buffer);
			tidyNodeGetValue(data->tidy_doc, child, &data->draft_buffer);
			if (data->draft_buffer.bp != NULL) {
				// Let's say the expression
				//     data->draft_buffer.size == strlen(data->draft_buffer.bp)
				// is always true. So far it has always worked.
				cpyas(data->value, (char *)data->draft_buffer.bp, data->draft_buffer.size);
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
				tag_namespace = find_namespace_uri_by_its_name(&data->namespaces, tag_name, sep_pos - tag_name);
				tag_name = sep_pos + 1;
			} else if (data->def_ns != NULL) {
				tag_namespace = data->def_ns->uri;
			}

			stuff_to_do_when_xml_element_starts(data, tag_namespace, tag_name, attrs, sep_pos == NULL ? false : true);

			if (dumpNode(data, child) == false) {
				return false;
			}

			stuff_to_do_when_xml_element_ends(data, tag_namespace, tag_name, attrs, sep_pos == NULL ? false : true);

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
	TidyBuffer error_buffer;
	tidyBufInit(&error_buffer);
	tidySetErrorBuffer(data->tidy_doc, &error_buffer);
	tidyOptSetBool(data->tidy_doc, TidyXmlTags, true); // Enable XML mode.
	tidyOptSetBool(data->tidy_doc, TidyXmlSpace, true);
	tidyParseString(data->tidy_doc, feed_buf->ptr);
	tidyCleanAndRepair(data->tidy_doc);
	int tidy_status = tidyRunDiagnostics(data->tidy_doc);

	if (error_buffer.bp != NULL) {
		INFO("Tidy's report:\n%s", error_buffer.bp);
	} else {
		// Shouldn't happen.
		WARN("Tidy's error buffer is nullified!");
	}

	tidyBufFree(&error_buffer);

	if (tidy_status != 0) {
		WARN("Tidy can't continue to process this buffer because it has critical errors!");
		return false;
	}

	if (dumpNode(data, tidyGetRoot(data->tidy_doc)) == false) {
		FAIL("Something really bad happened during XML parsing!");
		return false;
	}

	return true;
}

bool
parse_xml_feed(const struct string *feed_buf, struct getfeed_feed *feed)
{
	struct xml_data data;
	if ((data.value = crtes()) == NULL) {
		FAIL("Not enough memory for updating a feed!");
		return false;
	}
	data.feed = feed;
	data.def_ns = NULL;
	data.namespaces.top = 0;
	data.namespaces.lim = 0;
	data.namespaces.buf = NULL;
	data.depth = 0;
	for (size_t i = 0; i < XML_FORMATS_COUNT; ++i) {
		data.xml_pos[i] = 0;
	}
	data.default_handler = XML_FORMATS_COUNT;
	data.tidy_doc = tidyCreate();
	tidyBufInit(&data.draft_buffer);
	data.error = PARSE_OKAY;

	if (enter_xml_parsing_loop(feed_buf, &data) == false) {
		WARN("Feed update stopped due to parsing failure!");
		tidyBufFree(&data.draft_buffer);
		tidyRelease(data.tidy_doc);
		free_string(data.value);
		return false;
	}

	tidyBufFree(&data.draft_buffer);
	tidyRelease(data.tidy_doc);
	free_default_namespaces(data.def_ns);
	free_namespace_stack(&data.namespaces);
	free_string(data.value);

	return true;
}
