#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <expat.h>
#include <sqlite3.h>
#include <time.h>
#include <wchar.h>
#define LENGTH(A) (sizeof(A)/sizeof(*A))
#define DEBUG_WRITE_DB_PREPARE_FAIL debug_write(DBG_WARN, "failed to prepare statement: %s\n", sqlite3_errmsg(db))
#define NAMESPACE_SEPARATOR ' '

struct string {
	char *ptr;
	size_t len;
	size_t lim;
};

struct wstring {
	wchar_t *ptr;
	size_t len;
	size_t lim;
};

struct feed_tag {
	char *name;
	struct string **urls;
	size_t urls_count;
};

struct set_condition {
	struct string *db_cmd; // WHERE condition string
	struct string **urls;  // array of urls to replace the placeholders in db_cmd
	size_t urls_count;     // number of elements in urls array
};

struct set_line {
	struct string *name; // what is displayed in menu
	struct string *link; // this is feed url if set is feed
	struct string *tags; // this is tags expression if set is filter
	size_t unread_count;
	WINDOW *window;
};

struct item_line {
	struct string *title;
	struct string *feed_url;
	bool is_unread;
	WINDOW *window;
	int rowid;               // id of row related to this item
};

struct parser_data {
	char *value;
	size_t value_len;
	size_t value_lim;
	int depth;
	int pos;
	int prev_pos;
	const struct string *feed_url;
	struct item_bucket *bucket;
	XML_Parser *parser;
	bool fail;
};

struct author {
	struct string *name;
	struct string *email;
	struct string *link;
};

// used to bufferize item before writing to disk
// so we can reject it in case parsed item is already cached
struct item_bucket {
	struct string *guid;
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *comments;
	struct string *categories;
	struct author *authors;
	size_t authors_count;
	time_t pubdate;
	time_t upddate;
};

enum items_column {
	ITEM_COLUMN_FEED,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_URL,
	ITEM_COLUMN_AUTHORS,
	ITEM_COLUMN_CATEGORIES,
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
	IN_NAME_ELEMENT = 16384,
	IN_URL_ELEMENT = 32768,
	IN_EMAIL_ELEMENT = 65536,
};

enum debug_level {
	DBG_INFO = 0,
	DBG_OK = 1,
	DBG_WARN = 2,
	DBG_ERR = 3,
};

enum input_cmd {
	INPUT_SELECT_NEXT = 0,
	INPUT_SELECT_NEXT_PAGE,
	INPUT_SELECT_PREV,
	INPUT_SELECT_PREV_PAGE,
	INPUT_SELECT_FIRST,
	INPUT_SELECT_LAST,
	INPUT_ENTER,
	INPUT_RELOAD,
	INPUT_RELOAD_ALL,
	INPUT_QUIT_SOFT,
	INPUT_QUIT_HARD,
	INPUT_MARK_READ,
	INPUT_MARK_READ_ALL,
	INPUT_MARK_UNREAD,
	INPUT_MARK_UNREAD_ALL,
	INPUT_RESIZE,
	INPUTS_COUNT,
};

// list interface
int create_list_menu(void);
void resize_list_menu(void);
WINDOW *get_list_entry_by_index(size_t i);
void free_list_menu(void);

// sets
void enter_sets_menu_loop(void);
int load_sets(void);
void free_sets(void);
void print_set_format(size_t index, const struct set_line *set);
void resize_sets_global_action(void);

// items
int enter_items_menu_loop(const struct set_condition *st);
void print_item_format(size_t index, const struct item_line *item);
void resize_items_global_action(void);

// contents
int cat_item_meta_data_to_buf(struct string *buf, sqlite3_stmt *res);
struct string *expand_html_entities(char *buf, size_t buf_len);
struct string *plainify_html(char *buf, size_t buff_len);
int enter_item_contents_menu_loop(int rowid);

// path
int set_feeds_path(const char *path);
int set_config_path(const char *path);
int set_db_path(const char *path);
char *get_feeds_path(void);
char *get_config_path(void);
char *get_db_path(void);

// config processing
void free_config_data(void);
int assign_default_values_to_empty_config_strings(void);
int load_config(void);

// date parsing
time_t parse_date_rfc822(const char *date_str, size_t date_len);
time_t parse_date_rfc3339(const char *date_str, size_t date_len);
struct string *get_config_date_str(const time_t *time_ptr);

// feed parsing
int feed_process(const struct string *url);
void drop_item_bucket(struct item_bucket *bucket);

// tags
int tag_feed(char *tag_name, struct string *url);
// convert data of set to sql WHERE condition
struct set_condition * create_set_condition(struct set_line *set);
void free_set_condition(struct set_condition *cond);
void debug_tags_summary(void);
struct feed_tag *get_tag_by_name(char *name);
void free_tags(void);

// db
int db_init(void);
void db_stop(void);
int db_update_item_int(int rowid, const char *state, int value);
void db_update_feed_text(const struct string *feed_url, const char *column, const char *data, size_t data_len);
size_t get_unread_items_count_of_feed(const struct string *url);
void try_item_bucket(const struct item_bucket *bucket, const struct string *feed_url);

// functions related to window which displays informational messages (see status.c file)
int status_create(void);
void status_write(const char *format, ...);
void status_clean(void);
void status_delete(void);

// functions related to window which handles user input (see interface-input.c file)
void reset_input_handlers(void);
void set_input_handler(enum input_cmd, void (*func)(void));
int handle_input(void);
int assign_command_to_key(int bind_key, enum input_cmd bind_cmd);
int load_default_binds(void);
void free_binds(void);

// string
struct string *create_string(const char *src, size_t len);
struct string *create_empty_string(void);
void cpy_string_string(struct string *dest, const struct string *src);
void cpy_string_array(struct string *dest, const char *src_ptr, size_t src_len);
void cat_string_string(struct string *dest, const struct string *src);
void cat_string_array(struct string *dest, const char *src, size_t src_len);
void cat_string_char(struct string *dest, char c);
void empty_string(struct string *str);
void free_string(struct string *str);
void strip_whitespace_from_edges(char *str, size_t *len);
// wstring
struct wstring *convert_string_to_wstring(const struct string *src);
void free_wstring(struct wstring *wstr);

const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);

// item bucket functions
struct item_bucket *create_item_bucket(void);
void drop_item_bucket(struct item_bucket *bucket);
void free_item_bucket(struct item_bucket *bucket);
void add_category_to_item_bucket(const struct item_bucket *bucket, const char *category, size_t category_len);

// xml element handlers
int parse_namespace_element_beginning        (void *userData, const XML_Char *name, const XML_Char **atts);
int parse_namespace_element_end              (void *userData, const XML_Char *name);
void XMLCALL parse_rss20_element_beginning   (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_rss20_element_end         (void *userData, const XML_Char *name);
void XMLCALL parse_generic_element_beginning (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_generic_element_end       (void *userData, const XML_Char *name);
void XMLCALL parse_rss10_element_beginning   (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_rss10_element_end         (void *userData, const XML_Char *name);
void XMLCALL parse_atom10_element_beginning  (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_atom10_element_end        (void *userData, const XML_Char *name);
void XMLCALL parse_atom03_element_beginning  (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_atom03_element_end        (void *userData, const XML_Char *name);
void XMLCALL parse_dc_element_beginning      (void *userData, const XML_Char *name, const XML_Char **atts);
void XMLCALL parse_dc_element_end            (void *userData, const XML_Char *name);

// debug
int debug_init(const char *path);
void debug_write(enum debug_level lvl, const char *format, ...);
void debug_stop(void);

extern sqlite3 *db;
extern int list_menu_height;
extern int list_menu_width;

extern size_t config_max_items; // 0 == inf
extern size_t config_init_parser_buf_size;
extern char*  config_menu_set_entry_format;
extern char*  config_menu_item_entry_format;
extern char*  config_contents_meta_data;
extern char*  config_contents_date_format;
#endif // FEEDEATER_H
