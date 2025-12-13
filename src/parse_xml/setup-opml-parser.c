#include <string.h>
#include "newsraft.h"

static void
opml_start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct string *feeds = userData;
	if (strcmp(name, "outline") == 0) {
		const char *feed = get_value_of_attribute_key(atts, "xmlUrl");
		if (feed && strlen(feed) > 0) {
			catas(feeds, feed, strlen(feed));

			const char *name = get_value_of_attribute_key(atts, "title");
			if (name == NULL) {
				name = get_value_of_attribute_key(atts, "text");
			}
			if (name && strlen(name) > 0) {
				catas(feeds, " \"", 2);
				catas(feeds, name, strlen(name));
				catcs(feeds, '"');
			}

			catcs(feeds, '\n');
		}
	}
}

static void
opml_end_element_handler(void *userData, const XML_Char *name)
{
	(void)userData;
	(void)name;
	return;
}

bool
convert_opml_to_feeds(void)
{
	FILE *f = fopen("/dev/stdin", "r");
	if (f == NULL) {
		return false;
	}
	struct string *opml = crtes(10000);
	for (int c = fgetc(f); c != EOF; c = fgetc(f)) {
		catcs(opml, c);
	}
	if (opml->len == 0) {
		free_string(opml);
		return false;
	}
	XML_Parser parser = XML_ParserCreateNS(NULL, ' ');
	if (parser == NULL) {
		free_string(opml);
		return false;
	}
	struct string *feeds = crtes(50000);
	XML_SetUserData(parser, feeds);
	XML_SetElementHandler(parser, &opml_start_element_handler, &opml_end_element_handler);
	if (XML_Parse(parser, opml->ptr, opml->len, XML_TRUE) != XML_STATUS_OK) {
		write_error("XML parser failed: %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
		XML_ParserFree(parser);
		free_string(opml);
		free_string(feeds);
		return false;
	}
	fputs(feeds->ptr, stdout);
	fflush(stdout);
	XML_ParserFree(parser);
	free_string(opml);
	free_string(feeds);
	return true;
}

bool
convert_feeds_to_opml(void)
{
	size_t feeds_count = 0;
	struct feed_entry **feeds = get_all_feeds(&feeds_count);

	if (feeds_count == 0 || feeds == NULL) {
		return false;
	}

	struct string *opml = crtes(50000);
	const char *header =
		"<?xml version=\"1.0\"?>\n"
		"<opml version=\"2.0\">\n"
		"\t<head>\n\t\t<title>Newsraft - Exported Feeds</title>\n\t</head>\n"
		"\t<body>\n";
	const char *footer = "\t</body>\n</opml>\n";

	catas(opml, header, strlen(header));

	char *indent;
	size_t section_index = 0;
	char *section_name = NULL;
	while ((section_name = get_section_name(section_index)) != NULL) {

		if (section_index != 0) {
			str_appendf(opml, "\t\t<outline type=\"newsraft-section\" text=\"%s\">\n", section_name);
		}

		for (size_t i = 0; i < feeds_count; ++i) {
			if (feeds[i]->section_index != section_index) {
				continue; // skip feeds from other sections
			}
			if (feeds[i]->url->ptr[0] == '$') {
				continue; // skip command feeds
			}

			indent = section_index == 0 ? "\t\t" : "\t\t\t";

			if (STRING_IS_EMPTY(feeds[i]->name)) {
				str_appendf(opml, "%s<outline type=\"rss\" xmlUrl=\"%s\" />\n", indent, feeds[i]->url->ptr);
			} else {
				str_appendf(
					opml,
					"%s<outline type=\"rss\" xmlUrl=\"%s\" title=\"%s\" text=\"%s\" />\n",
					indent,
					feeds[i]->url->ptr,
					feeds[i]->name->ptr,
					feeds[i]->name->ptr
				);
			}
		}

		if (section_index != 0) {
			str_appendf(opml, "\t\t</outline>\n", section_name);
		}

		section_index++;
	}

	catas(opml, footer, strlen(footer));

	fputs(opml->ptr, stdout);
	fflush(stdout);

	free_string(opml);

	return true;
}
