#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <expat.h>
#include <sqlite3.h>
#include <time.h>
#define MAXPATH 1024
#define LENGTH(A) (sizeof(A)/sizeof(*A))

struct string {
	char *ptr;
	size_t len;
	size_t lim;
};

struct set_statement {
	struct string *db_cmd;
	struct string **urls;
	size_t urls_count;
};

struct set_line {
	struct string *name; // what is displayed in menu
	struct string *link; // this is feed url if set is feed
	struct string *tags; // this is tags expression if set is filter
	bool is_marked;
	bool is_unread;
	WINDOW *window;
};

struct item_entry {
	struct string *title; // name of item
	struct string *url;   // url to item
	struct string *guid;
};

struct item_line {
	struct item_entry *data;
	struct string *feed_url;
	bool is_marked;
	bool is_unread;
	WINDOW *window;
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
	int (*parser_func)(XML_Parser *parser);
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
	time_t upddate;
};

enum menu_dest {
	MENU_FEEDS,
	MENU_ITEMS,
	MENU_ITEMS_ERROR,
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
	ITEM_COLUMN_UPDDATE,
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
	IN_AUTHOR_ELEMENT = 64,
	IN_CATEGORY_ELEMENT = 128,
	IN_COMMENTS_ELEMENT = 256,
	IN_ENCLOSURE_ELEMENT = 512,
	IN_SOURCE_ELEMENT = 1024,
	IN_IMAGE_ELEMENT = 2048,
	IN_UPDDATE_ELEMENT = 4096,
	IN_CHANNEL_ELEMENT = 8192,
};

enum debug_level {
	DBG_INFO = 0,
	DBG_OK = 1,
	DBG_WARN = 2,
	DBG_ERR = 3,
};


int load_sets(void);
void free_sets(void);
int run_sets_menu(void);
void hide_sets(void);


// items

int run_items_menu(struct set_statement *st);
void hide_items(void);
void drop_item_bucket(struct item_bucket *bucket);
int try_item_bucket(struct item_bucket *bucket, struct string *feed_url);


// contents
struct string *expand_html_entities(char *buf, size_t buf_len);
struct string *plainify_html(char *buff, size_t buff_len);
int contents_menu(struct item_line *item);


// path
int set_conf_dir_path(void);
int set_data_dir_path(void);
void free_conf_dir_path(void);
void free_data_dir_path(void);
char * get_conf_file_path(char *file_name);
char * get_db_path(void);


// feed parsing
void value_strip_whitespace(char *str, size_t *len);
void XMLCALL store_xml_element_value(void *userData, const XML_Char *s, int s_len);
int feed_process(struct string *buf, struct string *url);
time_t get_unix_epoch_time(char *format_str, char *date_str);
int parse_generic(XML_Parser *parser);
int parse_rss20(XML_Parser *parser);

// tags
void tag_feed(char *tag_name, struct string *url);
// convert data of set to sql WHERE condition
struct set_statement * create_set_statement(struct set_line *set);
void debug_tags_summary(void);
void free_tags(void);


// db
int db_init(void);
void db_bind_string(sqlite3_stmt *s, int pos, struct string *str);
int db_update_item_int(struct string *feed_url, struct item_entry *item, const char *state, int value);
void db_update_feed_int64(struct string *feed_url, char *column, int64_t i);
void db_update_feed_text(struct string *feed_url, char *column, char *data, size_t data_len);
bool is_feed_marked(struct string *url);
bool is_feed_unread(struct string *url);
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
struct string *create_string(char *src, size_t len);
struct string *create_empty_string(void);
void cpy_string_string(struct string *dest, struct string *src);
void cpy_string_array(struct string *dest, char *src_ptr, size_t src_len);
void cpy_string_char(struct string *dest, char c);
void cat_string_string(struct string *dest, struct string *src);
void cat_string_array(struct string *dest, char *src, size_t src_len);
void cat_string_char(struct string *dest, char c);
void empty_string(struct string *str);
void free_string(struct string *str);

// debug
int debug_init(char *path);
void debug_write(enum debug_level lvl, char *format, ...);
void debug_stop(void);


// namespaces
int process_namespaced_tag_start(void *userData, const XML_Char *name, const XML_Char **atts);
int process_namespaced_tag_end(void *userData, const XML_Char *name);
void XMLCALL rss_10_start(void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL rss_10_end(void *userData, const XML_Char *name);
void XMLCALL atom_10_start(void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL atom_10_end(void *userData, const XML_Char *name);
void XMLCALL atom_03_start(void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL atom_03_end(void *userData, const XML_Char *name);


extern sqlite3 *db;
#endif // FEEDEATER_H
