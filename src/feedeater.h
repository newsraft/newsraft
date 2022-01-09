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

#define COUNTOF(A) (sizeof(A) / sizeof(*A))
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
	const struct string *tags; // this is tags expression if set is filter NULL otherwise
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
struct content_list {
	struct string *content;
	char *content_type;
	bool trim_whitespace;
	struct content_list *next;
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

struct config_data {
	size_t max_items;
	bool append_links;
	wchar_t *menu_set_entry_format;
	wchar_t *menu_item_entry_format;
	char *contents_meta_data;
	char *contents_date_format;
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
	FEED_COLUMN_NONE,
};

enum item_column {
	ITEM_COLUMN_FEED_URL,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_LINK,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_ENCLOSURES,
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
int pager_view(const struct content_list *data_list);
bool append_content(struct content_list **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len, bool trim_whitespace);
bool populate_content_list_with_data_of_item(struct content_list **data_list, sqlite3_stmt *res);
void free_content_list(struct content_list *list);
bool populate_link_list_with_links_of_item(struct link_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct link_list *links);
bool add_another_url_to_trim_link_list(struct link_list *links, char *url, size_t url_len);
void free_trim_link_list(const struct link_list *links);
bool append_links_to_contents(struct content_list **contents, struct link_list *links);
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
const struct set_condition *create_set_condition_for_filter(const struct feed_tag *head_tag, const struct string *tags_expr);
void free_set_condition(const struct set_condition *cond);

// db
bool db_init(void);
void db_stop(void);
int db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
bool db_begin_transaction(void);
bool db_commit_transaction(void);
bool db_rollback_transaction(void);
const char *db_error_string(void);
sqlite3_stmt *db_find_item_by_rowid(int rowid);
int db_mark_item_read(int rowid);
int db_mark_item_unread(int rowid);
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
// wstring
struct wstring *wcrtas(const wchar_t *src_ptr, size_t src_len);
struct wstring *wcrtes(void);
bool wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
bool wcatcs(struct wstring *dest, wchar_t c);
bool wcatss(struct wstring *dest, const struct wstring *src);
void empty_wstring(struct wstring *wstr);
void free_wstring(struct wstring *wstr);
void trim_whitespace_from_wstring(struct wstring *wstr);
struct string *convert_wstring_to_string(const struct wstring *src);

bool log_init(const char *path);
void log_stop(void);

struct string *convert_bytes_to_human_readable_size_string(const char *value);

// Common functions for processing XML.
struct xml_attribute *get_attribute_list_of_xml_tag(const struct wstring *tag);
const struct wstring *get_value_of_xml_attribute(const struct xml_attribute *atts, const wchar_t *attr);
void free_attribute_list_of_xml_tag(struct xml_attribute *atts);

// Download, process and store new items of feed.
// See "update_feed" directory for implementation.
bool update_feed(const struct string *url);

// See "extract_links" directory for implementation.
bool extract_links(const struct content_list *data_list, struct link_list *target);

// Convert series of texts of different formats to one big
// content string that can be written to pad window without
// additional splitting into lines or any other processing.
// See "render_data" directory for implementation.
struct wstring *render_data(const struct content_list *data_list);

extern FILE *log_stream;
extern size_t list_menu_height;
extern size_t list_menu_width;
extern struct config_data cfg;

#endif // FEEDEATER_H
