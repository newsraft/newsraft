#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <expat.h>
#include <sqlite3.h>
#include <time.h>
#define PREPARE_SELECT_FAIL "failed to prepare SELECT statement: %s\n"
#define PREPARE_INSERT_FAIL "failed to prepare INSERT statement: %s\n"
#define PREPARE_UPDATE_FAIL "failed to prepare UPDATE statement: %s\n"
#define PREPARE_DELETE_FAIL "failed to prepare DELETE statement: %s\n"
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

struct parser_data {
	char *value;
	size_t value_len;
	size_t value_lim;
	int depth;
	int pos;
	int prev_pos;
	struct string *feed_url;
	struct item_bucket *bucket;
	XML_Parser *parser;
	bool fail;
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

// sets
int load_sets(void);
void free_sets(void);
int run_sets_menu(void);
void hide_sets(void);

// items
int run_items_menu(struct set_statement *st);
void hide_items(void);

// contents
struct string *expand_html_entities(char *buf, size_t buf_len);
struct string *plainify_html(char *buff, size_t buff_len);
int contents_menu(struct item_line *item);

// path
int set_feeds_path(char *path);
int set_db_path(char *path);
char *get_feeds_path(void);
char *get_db_path(void);

// date parsing
time_t parse_date_rfc822(char *date_str, size_t date_len);
time_t parse_date_rfc3339(char *date_str, size_t date_len);
struct string *get_config_date_str(time_t *time_ptr);

// feed parsing
int feed_process(struct string *url);
void drop_item_bucket(struct item_bucket *bucket);
void value_strip_whitespace(char *str, size_t *len);

// tags
void tag_feed(char *tag_name, struct string *url);
// convert data of set to sql WHERE condition
struct set_statement * create_set_statement(struct set_line *set);
void debug_tags_summary(void);
void free_tags(void);

// db
int db_init(void);
void db_stop(void);
int db_bind_string(sqlite3_stmt *s, int pos, struct string *str);
int db_update_item_int(struct string *feed_url, struct item_entry *item, const char *state, int value);
void db_update_feed_text(struct string *feed_url, char *column, char *data, size_t data_len);
bool is_feed_marked(struct string *url);
bool is_feed_unread(struct string *url);
void try_item_bucket(struct item_bucket *bucket, struct string *feed_url);

// functions related to window which displays informational messages (see status.c file)
int status_create(void);
void status_write(char *format, ...);
void status_clean(void);
void status_delete(void);

// functions related to window which handles user input (see input.c file)
int input_create(void);
int input_wgetch(void);
void input_delete(void);

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
void rss_10_start(void *userData, const XML_Char *name, const XML_Char **atts);
void rss_10_end(void *userData, const XML_Char *name);
void rss_10_dc_start(void *userData, const XML_Char *name, const XML_Char **atts);
void rss_10_dc_end(void *userData, const XML_Char *name);
void atom_10_start(void *userData, const XML_Char *name, const XML_Char **atts);
void atom_10_end(void *userData, const XML_Char *name);
void atom_03_start(void *userData, const XML_Char *name, const XML_Char **atts);
void atom_03_end(void *userData, const XML_Char *name);
void elem_generic_start(void *userData, const XML_Char *name, const XML_Char **atts);
void elem_generic_finish(void *userData, const XML_Char *name);
void elem_rss20_start(void *userData, const XML_Char *name, const XML_Char **atts);
void elem_rss20_finish(void *userData, const XML_Char *name);

extern sqlite3 *db;
#endif // FEEDEATER_H
