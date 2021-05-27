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
#define INIT_PARSER_BUF_SIZE 1000
#define LENGTH(A) (sizeof(A)/sizeof(*A))
#define IS_WHITESPACE(A) (((A) == ' ') || ((A) == '\t') || ((A) == '\r') || ((A) == '\n'))
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
	struct string *name;
	struct string *feed_url; // url of feed file
	struct string *site_url; // url of resource site
};

struct feed_window {
	struct feed_entry *feed;
	bool is_marked;
	bool is_unread;
	WINDOW *window;
	int64_t index;
};

struct item_entry {
	struct string *title; // name of item
	struct string *url;   // url to item
	struct string *guid;
};

struct item_window {
	struct item_entry *item;
	bool is_marked;
	bool is_unread;
	WINDOW *window;
	int64_t index;
};

struct feed_parser_data {
	char *value;
	size_t value_len;
	size_t value_lim;
	int depth;
	int pos;
	int prev_pos;
	struct string *feed_url;
	struct item_bucket *bucket;
};

struct init_parser_data {
	int depth;
	XML_Parser *xml_parser;
	int (*parser_func)(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data);
};

// used to bufferize item before writing to disk
// so we can reject it in case parsed item is already cached
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

enum menu_dest {
	MENU_FEEDS,
	MENU_ITEMS,
	MENU_ITEMS_EMPTY,
	MENU_CONTENT,
	MENU_CONTENT_ERROR,
	MENU_QUIT,
};

enum items_column {
	ITEM_COLUMN_FEED,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_MARKED,
	ITEM_COLUMN_URL,
	ITEM_COLUMN_AUTHOR,
	ITEM_COLUMN_CATEGORY,
	ITEM_COLUMN_PUBDATE,
	ITEM_COLUMN_COMMENTS,
	ITEM_COLUMN_CONTENT,
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
	IN_LANGUAGE_ELEMENT = 4096,
	IN_LASTBUILDDATE_ELEMENT = 8192,
	IN_CHANNEL_ELEMENT = 16384,
};

enum debug_level {
	DBG_OK = 0,
	DBG_WARN = 1,
	DBG_ERROR = 2,
};


// feeds

int load_feed_list(void);  // load feeds information in memory
void free_feed_list(void);
int run_feeds_menu(void);
void hide_feeds(void);


// items

int run_items_menu(struct string *feed_url);
void hide_items(void);
void reset_item_bucket(struct item_bucket *bucket);
int try_item_bucket(struct item_bucket *bucket, struct string *feed_url);


// contents
int contents_menu(struct string *feed_url, struct item_entry *item);


// path
int set_conf_dir_path(void);
int set_data_dir_path(void);
void free_conf_dir_path(void);
void free_data_dir_path(void);
char * get_config_file_path(char *file_name);
char * get_db_path(void);


// feed parsing
void value_strip_whitespace(char *str, size_t *len);
void XMLCALL store_xml_element_value(void *userData, const XML_Char *s, int s_len);
int feed_process(struct string *buf, struct feed_entry *feed);
time_t get_unix_epoch_time(char *format_str, char *date_str);
int parse_generic(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data);
int parse_rss20(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data);
int start_namespaced_tag(void *userData, const XML_Char *name, const XML_Char **atts);
int end_namespaced_tag(void *userData, const XML_Char *name);


// files parsing utility functions (see config.c)

void skip_chars(FILE *file, char *cur_char, char *list);
void find_chars(FILE *file, char *cur_char, char *list);


// db
int db_init(void);
void db_bind_string(sqlite3_stmt *s, int pos, struct string *str);
int db_update_item_int(struct string *feed_url, struct item_entry *item, const char *state, int value);
void db_update_feed_int64(struct string *feed_url, char *column, int64_t i);
void db_update_feed_text(struct string *feed_url, char *column, char *data, size_t data_len);
void db_stop(void);

// functions related to window which displays informational messages (see status.c file)
int status_create(void);
void status_write(char *format, ...);
void status_clean(void);
void status_delete(void);

// functions related to window which handles user input (see input.c file)
int input_create(void);
int input_wgetch(void);
void input_delete(void);


// curl
struct string * feed_download(char *url);


// string
void make_string(struct string **dest, void *src, size_t len);
struct string * create_string(void);
void free_string(struct string **dest);
void cat_string_string(struct string *dest, struct string *src);
void cat_string_array(struct string *dest, char *src, size_t src_len);
void cat_string_char(struct string *dest, char c);

// debug
int debug_init(char *path);
void debug_write(enum debug_level lvl, char *format, ...);
void debug_stop(void);


extern sqlite3 *db;
#endif // FEEDEATER_H
