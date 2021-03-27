#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ncurses.h>
#include "feedeater.h"

static void XMLCALL
start_element(void *userData, const XML_Char *name, const XML_Char **atts) {
	/*(void)atts;*/
	struct init_parser_data *parser_data = userData;
	if (parser_data->depth == 0 && parser_data->parser_func == NULL) {
		uint8_t i;
		if (strcmp(name, "rss") == 0) {
			for (i = 0; atts[i] != NULL && strcmp(atts[i], "version") != 0; ++i);
			if (atts[i] != NULL) ++i;
			if (atts[i] != NULL) {
				if      (strcmp(atts[i], "2.0") == 0)  parser_data->parser_func = &parse_rss20;
				else if (strcmp(atts[i], "1.1") == 0)  parser_data->parser_func = &parse_rss11;
				else if (strcmp(atts[i], "1.0") == 0)  parser_data->parser_func = &parse_rss10;
				else if (strcmp(atts[i], "0.94") == 0) parser_data->parser_func = &parse_rss094;
				else if (strcmp(atts[i], "0.92") == 0) parser_data->parser_func = &parse_rss092;
				else if (strcmp(atts[i], "0.91") == 0) parser_data->parser_func = &parse_rss091;
				else if (strcmp(atts[i], "0.90") == 0) parser_data->parser_func = &parse_rss090;
			}
		} else if (strcmp(name, "feed") == 0) {
			for (i = 0; atts[i] != NULL && strcmp(atts[i], "xmlns") != 0; ++i);
			if (atts[i] != NULL) ++i;
			if (atts[i] == NULL) {
				for (i = 0; atts[i] != NULL && strcmp(atts[i], "xmlns:atom") != 0; ++i);
				if (atts[i] != NULL) ++i;
			}
			if (atts[i] != NULL) {
				if      (strcmp(atts[i], "http://www.w3.org/2005/Atom") == 0) parser_data->parser_func = &parse_atom10;
				else if (strcmp(atts[i], "http://purl.org/atom/ns#") == 0)    parser_data->parser_func = &parse_atom03;
			}
		} else if (strcmp(name, "RDF") == 0) {
			parser_data->parser_func = &parse_rss10;
		}
		if (parser_data->parser_func != NULL) XML_StopParser(*(parser_data->xml_parser), (XML_Bool)1);
	}
	++(parser_data->depth);
}

static void XMLCALL
end_element(void *userData, const XML_Char *name) {
	struct init_parser_data *parser_data = userData;
	--(parser_data->depth);
}

int
feed_process(struct string *buf, struct feed_entry *feed)
{
	status_write("[parsing] %s", feed->feed_url);

	int do_reload_win = 0;
	XML_Parser parser = XML_ParserCreate(NULL);
	struct init_parser_data parser_data = {0, &parser, NULL};
	XML_SetUserData(parser, &parser_data);
	XML_SetElementHandler(parser, start_element, end_element);
	
	if (XML_Parse(parser, buf->ptr, buf->len, 0) == XML_STATUS_ERROR) {
		/*status_write("%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
								 /*XML_ErrorString(XML_GetErrorCode(parser)),*/
								 /*XML_GetCurrentLineNumber(parser));*/
		status_write("[invalid format] %s", feed->feed_url); // bad xml document
		XML_ParserFree(parser);
		return 0;
	}
	if (parser_data.parser_func == NULL) {
		status_write("[unknown format] %s", feed->feed_url);
		return 0;
	}

	struct feed_entry *remote_feed = parser_data.parser_func(&parser, feed->feed_url);
	XML_ParserFree(parser);
	if (remote_feed == NULL) {
		/*status_write("[incorrect format] %s", feed->feed_url);*/
		return 0;
	}

	if (remote_feed->rname != NULL) import_feed_value(feed->feed_url, "title", remote_feed->rname);
	if (remote_feed->site_url != NULL) import_feed_value(feed->feed_url, "link", remote_feed->site_url);
	if (feed->rname == NULL && remote_feed->rname != NULL) {
		feed->rname = malloc(strlen(remote_feed->rname) + 1);
		strcpy(feed->rname, remote_feed->rname);
		do_reload_win = 1;
	} else if (feed->rname != NULL && remote_feed->rname != NULL &&
	           strcmp(feed->rname, remote_feed->rname) != 0)
	{
		feed->rname = realloc(feed->rname, strlen(remote_feed->rname) + 1);
		strcpy(feed->rname, remote_feed->rname);
		do_reload_win = 1;
	}

	status_clean();
	free_feed_entry(remote_feed);
	/*status_write("[done] %s", feed->feed_url);*/
	return do_reload_win;
}
