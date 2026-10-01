#include <string.h>
#include "newsraft.h"

struct opml_parser_context {
	XML_Parser parser;
	struct string *current_section_name;
	int outline_depth;
};

static void
opml_start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts)
{
	if (strcmp(name, "outline") != 0)
		return;

	struct opml_parser_context *ctx = userData;
	struct feed_entry feed = {0};

	const char *title = get_value_of_attribute_key(atts, "title");
	if (title == NULL) {
		title = get_value_of_attribute_key(atts, "text");
	}

	const char *xml_url = get_value_of_attribute_key(atts, "xmlUrl");

	if (xml_url && strlen(xml_url) > 0) {
		// Treat elements with a xmlUrl as feeds.
		feed.url = crtas(xml_url, strlen(xml_url));

		if (title && strlen(title) > 0) {
			feed.name = crtas(title, strlen(title));
		}

		int64_t section_index = 0;
		if (ctx->current_section_name) {
			section_index = make_sure_section_exists(ctx->current_section_name);
		}

		INFO("Copying '%s' into section #%lld for import", feed.url->ptr, section_index);
		copy_feed_to_section(&feed, section_index);
	} else if (ctx->current_section_name == NULL && title != NULL && strlen(title) > 0) {
		// Treat top level elements where xmlUrl is missing as a section category.
		INFO("Set current section for import '%s'", title);
		ctx->current_section_name = crtas(title, strlen(title));
	} else {
		// Ignore elements without a xmlUrl when we already have a section set.
		INFO("Ignoring outline element for import '%s'", title);
	}

	ctx->outline_depth++;

	free_string(feed.url);
	free_string(feed.name);
}

static void
opml_end_element_handler(void *userData, const XML_Char *name)
{
	if (strcmp(name, "outline") != 0)
		return;

	struct opml_parser_context *ctx = userData;

	if (ctx->outline_depth == 1 && ctx->current_section_name != NULL) {
		// Unset the current section when an end element at depth 1 is found.
		INFO("Unset current section for import '%s'", ctx->current_section_name->ptr);
		free_string(ctx->current_section_name);
		ctx->current_section_name = NULL;
	}

	ctx->outline_depth--;
}

static bool
has_feed_in_section(struct feed_entry **feeds, size_t feeds_count, int64_t section_index)
{
	bool has_feed = false;

	for (size_t i = 0; i < feeds_count; ++i) {
		if (feeds[i]->section_index == section_index) {
			has_feed = true;
			break;
		}
	}

	return has_feed;
}

static bool
write_feeds_file(void)
{
	size_t feeds_count = 0;
	struct feed_entry **feeds = get_all_feeds(&feeds_count);
	struct string *output = crtes(20000);

	if (feeds_count == 0 || feeds == NULL) {
		free_string(output);
		return false;
	}

	bool has_global_feed = has_feed_in_section(feeds, feeds_count, 0);

	for (int64_t section_index = 0; true; ++section_index) {
		char *section_name = get_section_name(section_index);
		if (!section_name) {
			break;
		}

		if (section_index > 0) {
			bool need_newline = (section_index == 1 && has_global_feed) || section_index > 1;
			str_appendf(output, "%s@ %s\n", need_newline ? "\n" : "", section_name);
		}

		for (size_t i = 0; i < feeds_count; ++i) {
			if (feeds[i]->section_index != section_index) {
				continue;
			}
			if (STRING_IS_EMPTY(feeds[i]->name)) {
				str_appendf(output, "%s\n", feeds[i]->url->ptr);
			} else {
				str_appendf(
					output,
					"%s \"%s\"\n",
					feeds[i]->url->ptr,
					feeds[i]->name->ptr
				);
			}
		}
	}

	fputs(output->ptr, stdout);
	fflush(stdout);

	free_string(output);

	return true;
}

bool
convert_opml_to_feeds(void)
{
	bool ok = false;
	struct string *opml = crtes(10000);
	struct opml_parser_context ctx = {
		.parser = XML_ParserCreateNS(NULL, ' '),
	};
	FILE *f = fopen("/dev/stdin", "r");

	if (!f || !ctx.parser) {
		goto cleanup;
	}

	for (int c = fgetc(f); c != EOF; c = fgetc(f)) {
		catcs(opml, c);
	}

	if (opml->len == 0) {
		goto cleanup;
	}

	const struct string* section_name = get_cfg_string(NULL, CFG_GLOBAL_SECTION_NAME);
	make_sure_section_exists(section_name);

	XML_SetUserData(ctx.parser, &ctx);
	XML_SetElementHandler(ctx.parser, &opml_start_element_handler, &opml_end_element_handler);
	if (XML_Parse(ctx.parser, opml->ptr, opml->len, XML_TRUE) != XML_STATUS_OK) {
		write_error("XML parser failed: %s\n", XML_ErrorString(XML_GetErrorCode(ctx.parser)));
		goto cleanup;
	}

	ok = write_feeds_file();

cleanup:
	XML_ParserFree(ctx.parser);
	free_string(opml);
	if (f) fclose(f);
	return ok;
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
	int64_t section_index = 0;
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
			catas(opml, "\t\t</outline>\n", 13);
		}

		section_index++;
	}

	catas(opml, footer, strlen(footer));

	fputs(opml->ptr, stdout);
	fflush(stdout);

	free_string(opml);

	return true;
}
