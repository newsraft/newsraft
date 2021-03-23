#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ncurses.h>
#include <expat.h>
#include "feedeater.h"
#include "parsers/parsers.h"
#ifdef XML_LARGE_SIZE
#  define XML_FMT_INT_MOD "ll"
#else
#  define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
#  include <wchar.h>
#  define XML_FMT_STR "ls"
#else
#  define XML_FMT_STR "s"
#endif

struct parsing_buffer {
	int depth;
	struct feed_entry *(*parser)(struct string *buf);
};

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	uint8_t i;
	struct parsing_buffer *data = userData;

	data->depth += 1;
	if (data->depth == 1) {
		if (strcmp(name, "rss") == 0) {
			for (i = 0; atts[i] != NULL && strcmp(atts[i], "version") != 0; ++i);
			++i;
			if (atts[i] != NULL) {
				if      (strcmp(atts[i], "2.0") == 0)  data->parser = &parse_rss20;
				else if (strcmp(atts[i], "1.1") == 0)  data->parser = &parse_rss11;
				else if (strcmp(atts[i], "1.0") == 0)  data->parser = &parse_rss10;
				else if (strcmp(atts[i], "0.94") == 0) data->parser = &parse_rss094;
				else if (strcmp(atts[i], "0.92") == 0) data->parser = &parse_rss092;
				else if (strcmp(atts[i], "0.91") == 0) data->parser = &parse_rss091;
				else if (strcmp(atts[i], "0.90") == 0) data->parser = &parse_rss090;
			}
		} else if (strcmp(name, "feed") == 0) {
			for (i = 0; atts[i] != NULL && strcmp(atts[i], "xmlns") != 0; ++i);
			++i;
			if (atts[i] == NULL) {
				for (i = 0; atts[i] != NULL && strcmp(atts[i], "xmlns:atom") != 0; ++i);
				++i;
			}
			if (atts[i] != NULL) {
				if      (strcmp(atts[i], "http://www.w3.org/2005/Atom") == 0) data->parser = &parse_atom10;
				else if (strcmp(atts[i], "http://purl.org/atom/ns#") == 0)    data->parser = &parse_atom03;
			}
		} else if (strcmp(name, "RDF") == 0) {
			data->parser = &parse_rss10;
		}
	}
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	struct parsing_buffer *data = userData;
	/*(void)name;*/
	data->depth -= 1;
}

void
feed_process(struct string *buf, struct feed_entry *feed)
{
	struct parsing_buffer parser_data = {0, NULL};
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &parser_data);
	XML_SetElementHandler(parser, startElement, endElement);
	
	if (XML_Parse(parser, buf->ptr, buf->len, 1) == XML_STATUS_ERROR) {
		fprintf(stderr, "%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",
						XML_ErrorString(XML_GetErrorCode(parser)),
						XML_GetCurrentLineNumber(parser));
	}
	XML_ParserFree(parser);
	if (parser_data.parser == NULL) {
		status_write("Unknown format %s", feed->url);
		return;
	}

	struct feed_entry *remote_feed_ver = parser_data.parser(buf);
}

void
feed_reload(struct feed_entry *feed)
{
	struct string *buf = feed_download(feed->url);
	if (buf == NULL) return;
	if (buf->ptr == NULL) { free(buf); return; }
	feed_process(buf, feed);
	free(buf->ptr);
	free(buf);
}

void
feed_reload_all(void)
{
	//under construction
	return;
}

void
feed_view(char *url)
{
	//under construction
	return;
}
