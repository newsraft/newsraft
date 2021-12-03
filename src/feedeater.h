#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <time.h>
#include <wchar.h>
#define APPLICATION_VERSION "0.0.0"
#define LENGTH(A) (sizeof(A)/sizeof(*A))

#define INFO(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[INFO] " A "\n", ##__VA_ARGS__); } } while (0)
#define WARN(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[WARN] " A "\n", ##__VA_ARGS__); } } while (0)
#define FAIL(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[FAIL] " A "\n", ##__VA_ARGS__); } } while (0)
#define FAIL_SQLITE_PREPARE FAIL("Failed to prepare an SQL statement: %s", sqlite3_errmsg(db))

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
	const struct string **urls;
	size_t urls_count;
};

struct set_condition {
	struct string *db_cmd;       // WHERE condition string
	const struct string **urls;  // array of urls to replace the placeholders in db_cmd
	size_t urls_count;           // number of elements in urls array
};

struct set_line {
	struct string *name; // what is displayed in menu
	struct string *link; // this is feed url if set is feed NULL otherwise
	struct string *tags; // this is tags expression if set is filter NULL otherwise
	const struct set_condition *cond;
	int unread_count;
	WINDOW *window;
};

struct item_line {
	struct string *title;
	bool is_unread;
	WINDOW *window;
	int rowid;               // id of row related to this item
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

enum item_column {
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
int pager_view(struct string *data);
int cat_item_meta_data_to_buf(struct string *buf, sqlite3_stmt *res);
struct string *expand_html_entities(const char *str, size_t str_len);
struct string *plainify_html(const char *str, size_t str_len);
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

// tags
int tag_feed(const char *tag_name, const struct string *url);
const struct set_condition *create_set_condition_for_feed(const struct string *feed_url);
const struct set_condition *create_set_condition_for_filter(const struct string *tags_expr);
void free_set_condition(const struct set_condition *cond);
const struct feed_tag *get_tag_by_name(const char *name);
void free_tags(void);

// db
int db_init(void);
void db_stop(void);
int db_mark_item_read(int rowid);
int db_mark_item_unread(int rowid);
int get_unread_items_count(const struct set_condition *sc);

int curses_init(void);

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
struct string *cpyas(struct string *dest, const char *src_ptr, size_t src_len);
struct string *cpyss(struct string *dest, const struct string *src);
struct string *catas(struct string *dest, const char *src, size_t src_len);
struct string *catss(struct string *dest, const struct string *src);
struct string *catcs(struct string *dest, char c);
void empty_string(struct string *str);
void free_string(struct string *str);
void strip_whitespace_from_edges(char *str, size_t *len);
// wstring
struct wstring *create_wstring(const wchar_t *src, size_t len);
struct wstring *create_empty_wstring(void);
void cat_wstring_array(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
void cat_wstring_wchar(struct wstring *dest, wchar_t wc);
struct wstring *convert_string_to_wstring(const struct string *src);
void free_wstring(struct wstring *wstr);

// debug
int log_init(const char *path);
void log_stop(void);

// Download, process and store a new items of feed.
// See "update_feed" directory for implementation.
int update_feed(const struct string *url);

extern FILE *log_stream;
extern sqlite3 *db;
extern int list_menu_height;
extern int list_menu_width;

extern size_t config_max_items; // 0 == inf
extern size_t config_init_parser_buf_size;
extern char*  config_menu_set_entry_format;
extern char*  config_menu_item_entry_format;
extern char*  config_contents_meta_data;
extern char*  config_contents_date_format;
extern char*  config_break_at;
#endif // FEEDEATER_H
