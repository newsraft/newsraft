#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <wchar.h>
#include <ncurses.h>
#include <sqlite3.h>

#define FEEDEATER_VERSION "0.0.0"
#define MAX_MIME_TYPE_LEN 255
// This has to be the length of the longest HTML entity name in entities array.
#define MAX_ENTITY_NAME_LENGTH 13

#define COUNTOF(A) (sizeof(A) / sizeof(*A))
#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWIDEWHITESPACE(A) (((A)==L' ')||((A)==L'\n')||((A)==L'\t')||((A)==L'\v')||((A)==L'\f')||((A)==L'\r'))
#define INFO(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[INFO] " A "\n", ##__VA_ARGS__); } } while (0)
#define WARN(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[WARN] " A "\n", ##__VA_ARGS__); } } while (0)
#define FAIL(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[FAIL] " A "\n", ##__VA_ARGS__); } } while (0)

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

// Linked list
struct feed_tag {
	char *name;
	const struct string **urls;
	size_t urls_count;
	struct feed_tag *next_tag;
};

struct set_condition {
	struct string *db_cmd;       // WHERE condition string
	const struct string **urls;  // array of urls to replace the placeholders in db_cmd
	size_t urls_count;           // number of elements in urls array
};

struct set_line {
	const struct string *name; // what is displayed in menu
	const struct string *link; // this is feed url if set is feed NULL otherwise
	const struct string *tags; // this is tags expression if set is multi-feed NULL otherwise
	const struct set_condition *cond;
	int unread_count;
	WINDOW *window;
};

struct item_line {
	struct string *title;
	bool is_unread;
	WINDOW *window;
	int rowid;            // id of row related to this item
};

struct format_arg {
	const wchar_t specifier;
	const wchar_t *const type_specifier;
	union {
		int i;
		char c;
		char *s;
		wchar_t *ls;
	} value;
};

// Linked list
struct render_block {
	struct wstring *content;
	char *content_type;
	struct render_block *next;
};

struct link {
	struct string *url;      // URL link to data.
	struct string *type;     // Standard MIME type of data.
	struct string *size;     // Size of data in bytes.
	struct string *duration; // Duration of data in seconds.
};

struct link_list {
	struct link *list; // Dynamic array of links.
	size_t len;        // Shows how many items is in list.
	size_t lim;        // Shows how many items list can fit.
};

struct xml_attribute {
	struct wstring *name;
	struct wstring *value;
};

enum xml_tag_pos {
	XML_TAG_ATTRIBUTE_NAME = 1,
	XML_TAG_ATTRIBUTE_VALUE_START = 2,
	XML_TAG_ATTRIBUTE_VALUE_QUOTED = 4,
	XML_TAG_ATTRIBUTE_VALUE_DOUBLE_QUOTED = 8,
	XML_TAG_ENTITY = 16,
};

struct xml_tag {
	struct wstring *buf;
	struct wstring *entity;
	struct xml_attribute *atts;
	size_t atts_len;
	enum xml_tag_pos pos;
};

struct config_data {
	size_t max_items;
	bool append_links;
	wchar_t *menu_set_entry_format;
	wchar_t *menu_item_entry_format;
	char *contents_meta_data;
	char *contents_date_format;
};

enum xml_tag_status {
	XML_TAG_FAIL,
	XML_TAG_CONTINUE,
	XML_TAG_DONE,
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

enum feed_column {
	FEED_COLUMN_FEED_URL,
	FEED_COLUMN_TITLE,
	FEED_COLUMN_LINK,
	FEED_COLUMN_SUMMARY,
	FEED_COLUMN_AUTHORS,
	FEED_COLUMN_EDITORS,
	FEED_COLUMN_WEBMASTERS,
	FEED_COLUMN_CATEGORIES,
	FEED_COLUMN_LANGUAGE,
	FEED_COLUMN_GENERATOR,
	FEED_COLUMN_RIGHTS,
	FEED_COLUMN_UPDATE_TIME,
	FEED_COLUMN_DOWNLOAD_TIME,
	FEED_COLUMN_NONE,
};

enum item_column {
	ITEM_COLUMN_FEED_URL,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_LINK,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_ATTACHMENTS,
	ITEM_COLUMN_AUTHORS,
	ITEM_COLUMN_CATEGORIES,
	ITEM_COLUMN_PUBDATE,
	ITEM_COLUMN_UPDDATE,
	ITEM_COLUMN_COMMENTS_URL,
	ITEM_COLUMN_SUMMARY,
	ITEM_COLUMN_CONTENT,
	ITEM_COLUMN_NONE,
};

// list interface
bool adjust_list_menu(void);
WINDOW *get_list_entry_by_index(size_t i);
void free_list_menu(void);
bool adjust_list_menu_format_buffer(void);
void free_list_menu_format_buffer(void);

// format
const wchar_t *do_format(const wchar_t *fmt, const struct format_arg *args, size_t args_count);

// sets
void enter_sets_menu_loop(void);
bool load_sets(void);
void free_sets(void);

// items
int enter_items_menu_loop(const struct set_condition *st);

// contents
int pager_view(const struct render_block *first_block);
bool join_render_block(struct render_block **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len);
bool join_render_separator(struct render_block **list);
bool join_render_blocks_of_item_data(struct render_block **data_list, sqlite3_stmt *res);
void free_render_blocks(struct render_block *first_block);
bool populate_link_list_with_links_of_item(struct link_list *links, sqlite3_stmt *res);
bool complete_urls_of_links(struct link_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct link_list *links);
bool add_another_url_to_trim_link_list(struct link_list *links, char *url, size_t url_len);
void free_trim_link_list(const struct link_list *links);
bool join_links_render_block(struct render_block **contents, struct link_list *links);
int enter_item_contents_menu_loop(int rowid);

// path
bool set_feeds_path(const char *path);
bool set_config_path(const char *path);
bool set_db_path(const char *path);
const char *get_feeds_path(void);
const char *get_config_path(void);
const char *get_db_path(void);
void free_feeds_path(void);
void free_config_path(void);
void free_db_path(void);

// config processing
void free_config_data(void);
bool assign_default_values_to_empty_config_strings(void);
bool load_config(void);

// date parsing
struct string *get_config_date_str(time_t time_ptr);

// tags
bool tag_feed(struct feed_tag **head_tag_ptr, const char *tag_name, size_t tag_name_len, const struct string *url);
const struct feed_tag *get_tag_by_name(const struct feed_tag *head_tag, const char *name);
void free_tags(struct feed_tag *head_tag);
const struct set_condition *create_set_condition_for_feed(const struct string *feed_url);
const struct set_condition *create_set_condition_for_multi_feed(const struct feed_tag *head_tag, const struct string *tags_expr);
void free_set_condition(const struct set_condition *cond);

// db
bool db_init(void);
void db_stop(void);
bool db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
bool db_begin_transaction(void);
bool db_commit_transaction(void);
bool db_rollback_transaction(void);
const char *db_error_string(void);
sqlite3_stmt *db_find_item_by_rowid(int rowid);
bool db_mark_item_read(int rowid);
bool db_mark_item_unread(int rowid);
struct string *db_get_plain_text_from_column(sqlite3_stmt *res, int column);
int get_unread_items_count(const struct set_condition *sc);

bool curses_init(void);
bool obtain_terminal_size(void);

// functions related to window which displays informational messages (see status.c file)
bool status_create(void);
void status_write(const char *format, ...);
void status_clean(void);
void status_delete(void);

// functions related to window which handles user input (see interface-input.c file)
void reset_input_handlers(void);
void set_input_handler(enum input_cmd, void (*func)(void));
int handle_input(void);
bool assign_action_to_key(int bind_key, enum input_cmd bind_cmd);
bool load_default_binds(void);
void free_binds(void);

// string
struct string *crtas(const char *src_ptr, size_t src_len);
struct string *crtss(const struct string *src);
struct string *crtes(void);
bool cpyas(struct string *dest, const char *src_ptr, size_t src_len);
bool cpyss(struct string *dest, const struct string *src);
bool catas(struct string *dest, const char *src_ptr, size_t src_len);
bool catss(struct string *dest, const struct string *src);
bool catcs(struct string *dest, char c);
void empty_string(struct string *str);
void free_string(struct string *str);
void trim_whitespace_from_string(struct string *str);
struct wstring *convert_string_to_wstring(const struct string *src);
struct string *convert_bytes_to_human_readable_size_string(const char *value);
// wstring
struct wstring *wcrtas(const wchar_t *src_ptr, size_t src_len);
struct wstring *wcrtss(const struct wstring *src);
struct wstring *wcrtes(void);
bool wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
bool wcpyss(struct wstring *dest, const struct wstring *src);
bool wcatcs(struct wstring *dest, wchar_t c);
bool wcatss(struct wstring *dest, const struct wstring *src);
void empty_wstring(struct wstring *wstr);
void free_wstring(struct wstring *wstr);
void trim_whitespace_from_wstring(struct wstring *wstr);
struct string *convert_wstring_to_string(const struct wstring *src);

bool log_init(const char *path);
void log_stop(void);

// Functions for processing XML tags.
struct xml_tag *create_tag(void);
enum xml_tag_status append_wchar_to_tag(struct xml_tag *tag, wchar_t wc);
bool append_array_to_tag(struct xml_tag *tag, const wchar_t *src, size_t src_len);
const struct wstring *get_value_of_xml_attribute(const struct xml_tag *tag, const wchar_t *attr);
void empty_tag(struct xml_tag *tag);
void free_tag(struct xml_tag *tag);

// Common functions for processing XML.
const wchar_t *translate_html_entity(wchar_t *entity);

// Download, process and store new items of feed.
// See "update_feed" directory for implementation.
bool update_feed(const struct string *url);

// Here we append links of HTML elements like <img> or <a> to link_list.
// Also, do some screen-independent processing of data that render blocks have
// (for example expand inline HTML elements like <sup>, <span> or <q>).
// See "prepare_to_render_data" directory for implementation.
bool prepare_to_render_data(struct render_block *first_block, struct link_list *links);

// Convert render blocks to one big string that can be written to pad window
// without additional splitting into lines or any other processing.
// See "render_data" directory for implementation.
struct wstring *render_data(const struct render_block *first_block);

extern FILE *log_stream;
extern size_t list_menu_height;
extern size_t list_menu_width;
extern struct config_data cfg;

#endif // FEEDEATER_H
