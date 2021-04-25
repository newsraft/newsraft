#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <expat.h>
#include <wchar.h>
#include <sqlite3.h>
#include <time.h>
#define MAXPATH 512
#define MAX_ITEM_INDEX_LEN 20
#define MAX_NAME_SIZE 128
#define MAX_URL_SIZE 256
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
	char *name;
	char *feed_url; // url of feed
	char *site_url; // url of site
	bool is_read;
};

struct feed_window {
	struct feed_entry *feed;
	WINDOW *window;
	int64_t index;
};

struct item_entry {
	char *name;    // name of item
	char *url;     // url to item
	char *guid;
	bool unread;
	int64_t index; // index of item
};

struct item_window {
	struct item_entry *item;
	WINDOW *window;
	int64_t index;
};

struct init_parser_data {
	int depth;
	XML_Parser *xml_parser;
	int (*parser_func)(XML_Parser *parser, char *url);
};

// used to bufferize item before writing to disk
// so we can reject it in case parsed item is already exist
struct item_bucket {
	struct string *guid;
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *author;
	struct string *category;
	struct string *comments;
	time_t pubdate;
};

enum xml_pos {
	IN_ROOT = 0,
	IN_ITEM_ELEMENT = 1,
	IN_TITLE_ELEMENT = 2,
	IN_DESCRIPTION_ELEMENT = 4,
	IN_LINK_ELEMENT = 8,
	IN_PUBDATE_ELEMENT = 16,
	IN_GUID_ELEMENT = 32,
	IN_CATEGORY_ELEMENT = 64,
	IN_COMMENTS_ELEMENT = 128,
	IN_AUTHOR_ELEMENT = 256,
	IN_ENCLOSURE_ELEMENT = 512,
	IN_SOURCE_ELEMENT = 1024,
	IN_IMAGE_ELEMENT = 2048,
	IN_CHANNEL_ELEMENT = 4096,
};

enum menu_dest {
	MENU_FEEDS,
	MENU_ITEMS,
	MENU_ITEMS_EMPTY,
	MENU_CONTENT,
	MENU_CONTENT_ERROR,
	MENU_EXIT,
};

enum items_column {
	ITEM_COLUMN_FEED,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_URL,
	ITEM_COLUMN_AUTHOR,
	ITEM_COLUMN_CATEGORY,
	ITEM_COLUMN_PUBDATE,
	ITEM_COLUMN_COMMENTS,
	ITEM_COLUMN_CONTENT,
};

struct feed_parser_data {
	char *value;
	size_t value_len;
	int depth;
	enum xml_pos pos;
	enum xml_pos last_pos;
	char *feed_url;
	struct item_bucket *bucket;
};


// feeds

int load_feed_list(void); // load feeds information in memory
void feeds_menu(void);    // display feeds in an interactive list


// items

int items_menu(char *feed_url);
void reset_item_bucket(struct item_bucket *bucket);
int try_item_bucket(struct item_bucket *bucket, char *feed_url);


// contents
int contents_menu(char *feed_url, struct item_entry *item);


// path
char * get_config_file_path(char *file_name);
char * get_db_path(void);


// feed parsing

int feed_process(struct string *buf, struct feed_entry *feed);

// xml parsers for different versions of feeds
int parse_rss20(XML_Parser *parser, char *feed_url);  // RSS 2.0
int parse_rss11(XML_Parser *parser, char *feed_url);  // RSS 1.1
int parse_rss10(XML_Parser *parser, char *feed_url);  // RSS 1.0
int parse_rss094(XML_Parser *parser, char *feed_url); // RSS 0.94
int parse_rss092(XML_Parser *parser, char *feed_url); // RSS 0.92
int parse_rss091(XML_Parser *parser, char *feed_url); // RSS 0.91
int parse_rss090(XML_Parser *parser, char *feed_url); // RSS 0.90
int parse_atom10(XML_Parser *parser, char *feed_url); // Atom 1.0
int parse_atom03(XML_Parser *parser, char *feed_url); // Atom 0.3


// config parsing

void skip_chars(FILE *file, char *cur_char, char *list);


// db
int db_init(void);
void db_bind_string(sqlite3_stmt *s, int pos, struct string *str);
int db_mark_item_unread(char *feed_url, struct item_entry *item, bool state);
void db_stop(void);

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
extern sqlite3 *db;


// curl
struct string * feed_download(char *url);


// utils
void make_string(struct string **dest, void *src, size_t len);
struct string * create_string(void);
void free_string(struct string **dest);
void cat_string_string(struct string *dest, struct string *src);
void cat_string_array(struct string *dest, char *src);
void cat_string_char(struct string *dest, char c);
#endif // FEEDEATER_H
