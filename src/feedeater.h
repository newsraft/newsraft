#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <ncurses.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <time.h>
#include <wchar.h>
#define FEEDEATER_VERSION "0.0.0"
#define LENGTH(A) (sizeof(A)/sizeof(*A))

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
	int rowid;               // id of row related to this item
};

union format_value {
	int i;
	char c;
	char *s;
};

struct format_arg {
	const char specifier;
	const char type_specifier;
	union format_value value;
};

// Linked list
struct content_list {
	struct string *content;
	char *content_type;
	struct content_list *next;
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
	ITEM_COLUMN_ENCLOSURES,
	ITEM_COLUMN_AUTHORS,
	ITEM_COLUMN_CATEGORIES,
	ITEM_COLUMN_PUBDATE,
	ITEM_COLUMN_UPDDATE,
	ITEM_COLUMN_COMMENTS,
	ITEM_COLUMN_SUMMARY,
	ITEM_COLUMN_SUMMARY_TYPE,
	ITEM_COLUMN_CONTENT,
	ITEM_COLUMN_CONTENT_TYPE,
	ITEM_COLUMN_NONE,
};

// list interface
int adjust_list_menu(void);
WINDOW *get_list_entry_by_index(size_t i);
void free_list_menu(void);

// format
int reallocate_format_buffer(void);
const char *do_format(char *fmt, struct format_arg *args, size_t args_count);
void free_format_buffer(void);

// sets
void enter_sets_menu_loop(void);
int load_sets(void);
void free_sets(void);

// items
int enter_items_menu_loop(const struct set_condition *st);

// contents
int pager_view(const struct content_list *data_list);
int append_content(struct content_list **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len);
int populate_content_list_with_data_of_item(struct content_list **data_list, sqlite3_stmt *res);
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
int assign_default_values_to_empty_config_strings(void);
int load_config(void);

// date parsing
struct string *get_config_date_str(const time_t *time_ptr);

// tags
int tag_feed(struct feed_tag **head_tag_ptr, const char *tag_name, size_t tag_name_len, const struct string *url);
const struct feed_tag *get_tag_by_name(const struct feed_tag *head_tag, const char *name);
void free_tags(struct feed_tag *head_tag);
const struct set_condition *create_set_condition_for_feed(const struct string *feed_url);
const struct set_condition *create_set_condition_for_filter(const struct feed_tag *head_tag, const struct string *tags_expr);
void free_set_condition(const struct set_condition *cond);

// db
int db_init(void);
void db_stop(void);
int db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
int db_begin_transaction(void);
int db_commit_transaction(void);
int db_rollback_transaction(void);
int db_mark_item_read(int rowid);
int db_mark_item_unread(int rowid);
int get_unread_items_count(const struct set_condition *sc);

int curses_init(void);
bool obtain_terminal_size(void);

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
int cpyas(struct string *dest, const char *src_ptr, size_t src_len);
int cpyss(struct string *dest, const struct string *src);
int catas(struct string *dest, const char *src, size_t src_len);
int catss(struct string *dest, const struct string *src);
int catcs(struct string *dest, char c);
void empty_string(struct string *str);
void free_string(struct string *str);
void strip_whitespace_from_string(struct string *str);
// wstring
struct wstring *create_wstring(const wchar_t *src, size_t len);
struct wstring *create_empty_wstring(void);
int wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
int wcatcs(struct wstring *dest, wchar_t c);
int wcatss(struct wstring *dest, const struct wstring *src);
struct wstring *convert_string_to_wstring(const struct string *src);
void empty_wstring(struct wstring *wstr);
void free_wstring(struct wstring *wstr);
void strip_whitespace_from_wstring(struct wstring *wstr);

int log_init(const char *path);
void log_stop(void);

struct content_list *create_content_list_for_item(int rowid);
void free_content_list(struct content_list *list);

bool is_wchar_a_breaker(wchar_t wc);

// Download, process and store a new items of feed.
// See "update_feed" directory for implementation.
int update_feed(const struct string *url);

// Convert series of texts of different formats to one big
// content string that can be written to pad window without
// additional splitting into lines or any other processing.
// See "render_data" directory for implementation.
struct wstring *render_data(const struct content_list *data_list);

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
