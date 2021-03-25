#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <string.h>
#include "feedeater.h"

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct parsing_buffer2 *data = userData;

	data->depth += 1;
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	struct parsing_buffer2 *data = userData;
	/*(void)name;*/
	data->depth -= 1;
}

struct feed_entry *
parse_rss20(struct string *buf)
{
	/*status_write("rss20 under construction");*/
	/*return NULL;*/
	struct feed_entry *feed = calloc(1, sizeof(struct feed_entry));
	if (feed == NULL) {
		status_write("calloc on feed buffer failed\n"); return NULL;
	}

	struct parsing_buffer2 feed_data = {0, feed};

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &feed_data);
	XML_SetElementHandler(parser, startElement, endElement);
	if (XML_Parse(parser, buf->ptr, buf->len, 1) == XML_STATUS_ERROR) {
		/*status_write("%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
								 /*XML_ErrorString(XML_GetErrorCode(parser)),*/
								 /*XML_GetCurrentLineNumber(parser));*/
		status_write("Invalid format %s", feed->url);
		XML_ParserFree(parser);
		return NULL;
	}
	XML_ParserFree(parser);

	return feed;
}
