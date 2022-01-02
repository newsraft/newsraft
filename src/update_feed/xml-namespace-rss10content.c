#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211201074403/https://web.resource.org/rss/1.0/modules/content/

static inline void
encoded_start(struct parser_data *data)
{
	data->rss10content_pos |= RSS10CONTENT_ENCODED;
}

static inline void
encoded_end(struct parser_data *data)
{
	if ((data->rss10content_pos & RSS10CONTENT_ENCODED) == 0) {
		return;
	}
	data->rss10content_pos &= ~RSS10CONTENT_ENCODED;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (data->item->content->len > data->value->len) {
		return;
	}
	if (cpyss(data->item->content, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyas(data->item->content_type, "text/html", 9) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_rss10content_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "encoded") == 0) { encoded_start(data); }
}

void
parse_rss10content_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "encoded") == 0) { encoded_end(data); }
}
#endif // FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
