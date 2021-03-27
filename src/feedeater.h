#include <ncurses.h>
#include <expat.h>
#include <wchar.h>
#define MAX_ITEMS 30
#define MAXPATH 512
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
	char *lname;    // local name of feed
	char *rname;    // remote name of feed
	char *feed_url; // url of feed
	char *site_url; // url of site
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
	struct feed_entry *(*parser_func)(XML_Parser *parser, char *url);
};

enum xml_pos {
	IN_ROOT = 0,
	IN_CHANNEL_ELEMENT = 1,
	IN_ITEM_ELEMENT = 2,
	IN_TITLE_ELEMENT = 4,
	IN_DESCRIPTION_ELEMENT = 8,
	IN_LINK_ELEMENT = 16,
	IN_AUTHOR_ELEMENT = 32,
	IN_PUBDATE_ELEMENT = 64,
	IN_CATEGORY_ELEMENT = 128,
	IN_ENCLOSURE_ELEMENT = 256,
	IN_COMMENTS_ELEMENT = 512,
	IN_SOURCE_ELEMENT = 1024,
	IN_IMAGE_ELEMENT = 2048,
};

enum menu_dest {
	MENU_FEEDS,
	MENU_ITEMS,
	MENU_CONTENTS,
	MENU_EXIT,
};

struct feed_parser_data {
	int depth;
	int items_count;
	struct feed_entry *feed;
	enum xml_pos pos;
	char *item_path;
};

int import_feed_value(char *url, char *element, char *value);
char *export_feed_value(char *url, char *element);
char *feed_item_data_path(char *url, int num);
void write_feed_item_elem(char *item_path, char *item_name, void *data, size_t size);

int load_feed_list(void);  // load feeds information in memory
void free_feed_list(void); // unload all feeds information (call this before exitting)
void feeds_menu(void);     // display feeds in an interactive list
void hide_feeds(void);     // hide interactive list of feeds
void items_menu(char *url);
char *get_config_file_path(char *file_name);
char *get_data_dir_path_for_url(char *url);

void free_feed_entry(struct feed_entry *);

struct string *feed_download(char *url);

int feed_process(struct string *buf, struct feed_entry *feed);


// parsing

void skip_chars(FILE *file, char *cur_char, char *list);

// xml parsers for different versions of feeds
struct feed_entry *parse_rss20(XML_Parser *parser, char *url);  // RSS 2.0
struct feed_entry *parse_rss11(XML_Parser *parser, char *url);  // RSS 1.1
struct feed_entry *parse_rss10(XML_Parser *parser, char *url);  // RSS 1.0
struct feed_entry *parse_rss094(XML_Parser *parser, char *url); // RSS 0.94
struct feed_entry *parse_rss092(XML_Parser *parser, char *url); // RSS 0.92
struct feed_entry *parse_rss091(XML_Parser *parser, char *url); // RSS 0.91
struct feed_entry *parse_rss090(XML_Parser *parser, char *url); // RSS 0.90
struct feed_entry *parse_atom10(XML_Parser *parser, char *url); // Atom 1.0
struct feed_entry *parse_atom03(XML_Parser *parser, char *url); // Atom 0.3


// ncurses

// functions related to window which displays informational messages (see status.c file)
int status_create(void);
void status_write(char *format, ...);
void status_clean(void);
int status_delete(void);

// functions related to window which handles user input (see input.c file)
int input_create(void);
int input_delete(void);
// variable with input window
extern WINDOW *input_win;
