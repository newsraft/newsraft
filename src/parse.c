#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ncurses.h>
#include "feedeater.h"
#include "config.h"

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
feed_process(struct buf *buf, struct feed_entry *feed)
{
	status_write("[parsing] %s", feed->feed_url);

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
		return 1;
	}
	if (parser_data.parser_func == NULL) {
		status_write("[unknown format] %s", feed->feed_url);
		return 1;
	}

	int parsing_done = parser_data.parser_func(&parser, feed->path);
	XML_ParserFree(parser);
	if (parsing_done == 0) {
		status_write("[incorrect format] %s", feed->feed_url);
		return 1;
	}

	return 0;
}

void
free_item_bucket(struct item_bucket *bucket)
{
	if (bucket == NULL) return;
	free_string(&bucket->uid);
	free_string(&bucket->title);
	free_string(&bucket->link);
	free_string(&bucket->content);
	free_string(&bucket->author);
	free_string(&bucket->category);
	free_string(&bucket->comments);
	free_string(&bucket->pubdate);
}

// as for now we checking uniqueness by uid file (guid tag value in rss20)
// if uid file does not exist, compare link files (link tag value in rss20)
int
is_item_unique(char *feed_path, struct item_bucket *bucket)
{
	char *item_path;
	struct buf *str;
	for (int64_t i = 0; i < config_max_items; ++i) {
		item_path = item_data_path(feed_path, i);
		if (item_path == NULL) continue;
		str = read_item_element(item_path, UNIQUE_ID_FILE);
		if (str != NULL) {
			if (is_bufs_equal(&bucket->uid, str)) {
				free_string_ptr(str);
				free(item_path);
				return 0;
			}
			free_string_ptr(str);
		} else {
			str = read_item_element(item_path, LINK_FILE);
			if (str != NULL) {
				if (is_bufs_equal(&bucket->link, str)) {
					free_string_ptr(str);
					free(item_path);
					return 0;
				}
				free_string_ptr(str);
			}
		}
		free(item_path);
	}
	return 1;
}

void
take_item_bucket(struct item_bucket *bucket, char *item_path)
{
	if (item_path == NULL || bucket == NULL) return;
	write_item_element(item_path, UNIQUE_ID_FILE, bucket->uid.ptr, bucket->uid.len);
	write_item_element(item_path, TITLE_FILE, bucket->title.ptr, bucket->title.len);
	write_item_element(item_path, LINK_FILE, bucket->link.ptr, bucket->link.len);
	write_item_element(item_path, PUBDATE_FILE, bucket->pubdate.ptr, bucket->pubdate.len);
	write_item_element(item_path, AUTHOR_FILE, bucket->author.ptr, bucket->author.len);
	write_item_element(item_path, CONTENT_FILE, bucket->content.ptr, bucket->content.len);
	write_item_element(item_path, CATEGORY_FILE, bucket->category.ptr, bucket->category.len);
	write_item_element(item_path, COMMENTS_FILE, bucket->comments.ptr, bucket->comments.len);
	mark_unread(item_path);
}
