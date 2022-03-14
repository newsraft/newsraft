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

static inline uint16_t
add_namespaces_to_stack(struct xml_namespace_stack *stack, const TidyAttr attrs)
{
	uint16_t added_count = 0;
	const char *attr_name;
	const char *namespace_name;
	const char *namespace_uri;
	for (TidyAttr attr = attrs; attr != NULL; attr = tidyAttrNext(attr)) {
		attr_name = tidyAttrName(attr);
		if (attr_name == NULL) {
			continue;
		}
		if (strstr(attr_name, "xmlns:") == attr_name) {
			namespace_name = attr_name + 6;
			namespace_uri = tidyAttrValue(attr);
			if (add_namespace_to_stack(stack, namespace_name, namespace_uri) == false) {
				return added_count;
			}
			added_count++;
		} else if (strstr(attr_name, "xmlns") == attr_name) {
			namespace_uri = tidyAttrValue(attr);
			if (namespace_uri != NULL && stack->defaultns == NULL) {
				stack->defaultns = crtas(namespace_uri, strlen(namespace_uri));
			}
		}
	}
	return added_count;
}

static void
start_element_handler(struct xml_data *data, const char *name, const TidyAttr atts)
{
	++(data->depth);
	empty_string(data->value);
	if (parse_namespace_element_start(data, name, atts) == true) {
		// Successfully processed an element by its namespace.
		return;
	}
	if (data->start_handler != NULL) {
		data->start_handler(data, name, atts);
		return;
	}
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	if ((data->depth == 1) && (strcmp(name, "rss") == 0)) {
		const char *version = get_value_of_attribute_key(atts, "version");
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

static void
end_element_handler(struct xml_data *data, const char *name)
{
	--(data->depth);
	trim_whitespace_from_string(data->value);
	if (parse_namespace_element_end(data, name) == true) {
		// Successfully processed an element by its namespace.
		return;
	}
	if (data->end_handler != NULL) {
		data->end_handler(data, name);
		return;
	}
}

static void
dumpNode(TidyDoc *tdoc, TidyNode tnod, TidyBuffer *buf, struct xml_data *data)
{
	uint16_t added_namespaces_count;
	const char *child_name;
	TidyNodeType child_type;
	TidyAttr child_attrs;
	for (TidyNode child = tidyGetChild(tnod); child != NULL; child = tidyGetNext(child)) {
		child_type = tidyNodeGetType(child);
		if ((child_type == TidyNode_Text) || (child_type == TidyNode_CDATA)) {
			tidyBufClear(buf);
			tidyNodeGetValue(*tdoc, child, buf);
			if (buf->bp != NULL) {
				catas(data->value, (char *)buf->bp, strlen((char *)buf->bp));
			}
		} else if ((child_type == TidyNode_Start) || (child_type == TidyNode_StartEnd)) {
			child_name = tidyNodeGetName(child);
			child_attrs = tidyAttrFirst(child);
			added_namespaces_count = add_namespaces_to_stack(&data->namespaces, child_attrs);
			start_element_handler(data, child_name, child_attrs);
			dumpNode(tdoc, child, buf, data);
			end_element_handler(data, child_name);
			for (uint16_t i = 0; i < added_namespaces_count; ++i) {
				pop_namespace_from_stack(&data->namespaces);
			}
		}
	}
}

static bool
enter_xml_parsing_loop(const struct string *feed_buf, struct xml_data *data)
{
	TidyDoc tdoc = tidyCreate();
	TidyBuffer tempbuf = {0};
	TidyBuffer tidy_errbuf = {0};

	tidyBufInit(&tempbuf);

	tidySetErrorBuffer(tdoc, &tidy_errbuf);
	tidyOptSetBool(tdoc, TidyXmlTags, true); // Enable XML mode.
	/* tidyOptSetBool(tdoc, TidyMakeBare, true); // use plain quotes instead fancy ones */
	/* tidyOptSetBool(tdoc, TidyOmitOptionalTags, false); */
	/* tidyOptSetBool(tdoc, TidyCoerceEndTags, true); */

	// Don't convert entities to characters.
	/* tidyOptSetBool(tdoc, TidyAsciiChars, false); */
	/* tidyOptSetBool(tdoc, TidyQuoteNbsp, true); */
	/* tidyOptSetBool(tdoc, TidyQuoteMarks, true); */
	/* tidyOptSetBool(tdoc, TidyQuoteAmpersand, true); */
	/* tidyOptSetBool(tdoc, TidyPreserveEntities, true); */

	tidyParseString(tdoc, feed_buf->ptr);
	tidyCleanAndRepair(tdoc);
	tidyRunDiagnostics(tdoc);

	dumpNode(&tdoc, tidyGetRoot(tdoc), &tempbuf, data);

	if (tidy_errbuf.bp != NULL) {
		INFO("Tidy report:\n%s", tidy_errbuf.bp);
	} else {
		INFO("Tidy's error buffer is NULL.");
	}

	tidyBufFree(&tempbuf);
	tidyBufFree(&tidy_errbuf);
	tidyRelease(tdoc);

	return true;
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
	data.namespaces.top = 0;
	data.namespaces.lim = 0;
	data.namespaces.buf = NULL;
	data.namespaces.defaultns = NULL;
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

	free_namespace_stack(&data.namespaces);
	free_string(data.value);

	return data.feed;
}
