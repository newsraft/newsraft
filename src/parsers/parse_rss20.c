#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct feed_parser_data *data = userData;

	data->depth += 1;
}

static void XMLCALL
endElement(void *userData, const XML_Char *name) {
	struct feed_parser_data *data = userData;
	/*(void)name;*/
	data->depth -= 1;
}

struct feed_entry *
parse_rss20(XML_Parser *parser, struct string *buf)
{
	/*status_write("rss20 beign parsed");*/
	/*return NULL;*/
	struct feed_entry *feed = calloc(1, sizeof(struct feed_entry));
	if (feed == NULL) {
		status_write("calloc on feed buffer failed\n"); return NULL;
	}

	struct feed_parser_data feed_data = {0, feed, 0};

	XML_SetUserData(*parser, &feed_data);
	XML_SetElementHandler(*parser, startElement, endElement);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
		return NULL;
	}

	return feed;
}
