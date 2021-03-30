#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <expat.h>
#include <wchar.h>
#define MAXPATH 512
#define MAX_ITEM_INDEX_LEN 20
#ifndef XML_LARGE_SIZE
#define XML_LARGE_SIZE
#endif
#ifndef XML_UNICODE_WCHAR_T
#define XML_UNICODE_WCHAR_T
#endif
#ifndef XML_FMT_INT_MOD
#define XML_FMT_INT_MOD "ll"
#endif
#ifndef XML_FMT_INT_MOD
#define XML_FMT_INT_MOD "l"
#endif
#ifndef XML_FMT_STR
#define XML_FMT_STR "ls"
#endif
#ifndef XML_FMT_STR
#define XML_FMT_STR "s"
#endif


struct string {
	char *ptr;
	size_t len;
};

struct feed_entry {
	char *name;     // remote name of feed
	char *feed_url; // url of feed
	char *site_url; // url of site
	char *path;     // path to data directory
};

struct feed_window {
	struct feed_entry *feed;
	WINDOW *window;
};

struct item_entry {
	char *name; // name of item
	char *url;  // url to item
};

struct item_window {
	struct item_entry *item;
	WINDOW *window;
};

struct init_parser_data {
	int depth;
	XML_Parser *xml_parser;
	int (*parser_func)(XML_Parser *parser, char *url);
};

enum xml_pos {
	IN_ROOT = 0,
	IN_ITEM_ELEMENT = 1,
	IN_TITLE_ELEMENT = 2,
	IN_DESCRIPTION_ELEMENT = 4,
	IN_LINK_ELEMENT = 8,
	IN_CATEGORY_ELEMENT = 16,
	IN_COMMENTS_ELEMENT = 32,
	IN_AUTHOR_ELEMENT = 64,
	IN_PUBDATE_ELEMENT = 128,
	IN_ENCLOSURE_ELEMENT = 256,
	IN_SOURCE_ELEMENT = 512,
	IN_IMAGE_ELEMENT = 1024,
	IN_CHANNEL_ELEMENT = 2048,
};

enum menu_dest {
	MENU_FEEDS,
	MENU_ITEMS,
	MENU_CONTENTS,
	MENU_EXIT,
};

struct feed_parser_data {
	int depth;
	int64_t item_index;
	enum xml_pos pos;
	char *feed_path;
	char *item_path;
	int64_t border_index;
	bool past_line;
};

void set_last_item_index(char *feed_path, int64_t index);
int64_t get_last_item_index(char *feed_path);
char *item_data_path(char *feed_path, int64_t num);
char *export_feed_value(char *url, char *element);
void write_feed_element(char *feed_path, char *element, void *data, size_t size);
void write_item_element(char *item_path, char *element, void *data, size_t size);

void contents_menu(void);
int load_feed_list(void);  // load feeds information in memory
void feeds_menu(void);     // display feeds in an interactive list
int items_menu(char *url);
char *get_config_file_path(char *file_name);
char *make_feed_dir(char *url);

struct string *feed_download(char *url);

int feed_process(struct string *buf, struct feed_entry *feed);


// parsing

void skip_chars(FILE *file, char *cur_char, char *list);

// xml parsers for different versions of feeds
int parse_rss20(XML_Parser *parser, char *feed_path);  // RSS 2.0
int parse_rss11(XML_Parser *parser, char *feed_path);  // RSS 1.1
int parse_rss10(XML_Parser *parser, char *feed_path);  // RSS 1.0
int parse_rss094(XML_Parser *parser, char *feed_path); // RSS 0.94
int parse_rss092(XML_Parser *parser, char *feed_path); // RSS 0.92
int parse_rss091(XML_Parser *parser, char *feed_path); // RSS 0.91
int parse_rss090(XML_Parser *parser, char *feed_path); // RSS 0.90
int parse_atom10(XML_Parser *parser, char *feed_path); // Atom 1.0
int parse_atom03(XML_Parser *parser, char *feed_path); // Atom 0.3


// ncurses

// functions related to window which displays informational messages (see status.c file)
int status_create(void);
void status_write(char *format, ...);
void status_clean(void);
void status_delete(void);

// functions related to window which handles user input (see input.c file)
int input_create(void);
void input_delete(void);
// variable with input window
extern WINDOW *input_win;
#endif // FEEDEATER_H
