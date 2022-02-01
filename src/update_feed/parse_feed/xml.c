#include <string.h>
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
get_value_of_attribute_key(const XML_Char **atts, const char *key)
{
	/* expat says that atts is name/value array (that is {name1, value1, name2, value2, ...})
	 * hence do iterate with the step of 2 */
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], key) == 0) {
			return atts[i + 1]; // success
		}
	}
	return NULL; // failure, didn't find an attribute with key name
}

static void
start_element_handler(struct xml_data *data, const XML_Char *name, const XML_Char **atts)
{
	if (data->error != PARSE_OKAY) {
		XML_StopParser(data->parser, XML_FALSE);
		return;
	}
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
end_element_handler(struct xml_data *data, const XML_Char *name)
{
	if (data->error != PARSE_OKAY) {
		XML_StopParser(data->parser, XML_FALSE);
		return;
	}
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

// Important note: a single block of contiguous text free of markup may still result in a sequence of calls to CharacterDataHandler.
static void
character_data_handler(struct xml_data *data, const XML_Char *s, int s_len)
{
	if (data->error != PARSE_OKAY) {
		XML_StopParser(data->parser, XML_FALSE);
		return;
	}
	if (catas(data->value, s, s_len) == false) {
		FAIL("Not enough memory for character data of the XML element!");
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
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

	if ((data.parser = XML_ParserCreateNS(NULL, XML_NAMESPACE_SEPARATOR)) == NULL) {
		FAIL("Something went wrong during parser struct creation, can not start updating a feed!");
		free_string(data.value);
		free_feed(data.feed);
		return NULL;
	}
	XML_SetElementHandler(
		data.parser,
		(void (*)(void *, const XML_Char *, const XML_Char **))&start_element_handler,
		(void (*)(void *, const XML_Char *))&end_element_handler
	);
	XML_SetCharacterDataHandler(
		data.parser,
		(void (*)(void *, const XML_Char *, int))&character_data_handler
	);
	XML_SetUserData(data.parser, &data);

	if (XML_Parse(data.parser, feed_buf->ptr, feed_buf->len, XML_TRUE) != XML_STATUS_OK) {
		FAIL("Feed update stopped due to parse error!");
		FAIL("Parser reports \"%" XML_FMT_STR "\" at line %" XML_FMT_INT_MOD "u.",
		     XML_ErrorString(XML_GetErrorCode(data.parser)),
		     XML_GetCurrentLineNumber(data.parser));
		free_string(data.value);
		free_feed(data.feed);
		XML_ParserFree(data.parser);
		return NULL;
	}

	XML_ParsingStatus status;
	XML_GetParsingStatus(data.parser, &status);
	if (status.parsing == XML_SUSPENDED) {
		free_string(data.value);
		free_feed(data.feed);
		XML_ParserFree(data.parser);
		return NULL;
	}

	free_string(data.value);
	XML_ParserFree(data.parser);

	return data.feed;
}
