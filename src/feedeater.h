#ifndef FEEDEATER_H
#define FEEDEATER_H
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>
#include <wchar.h>
#include <ncurses.h>
#include <sqlite3.h>

#define FEEDEATER_VERSION "0.0.0"
#define MAX_MIME_TYPE_LEN 255

#define COUNTOF(A) (sizeof(A) / sizeof(*A))
#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWHITESPACEEXCEPTNEWLINE(A) (((A)==' ')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
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

struct menu_list_settings {
	size_t entries_count;
	size_t view_sel;
	size_t view_min;
	size_t view_max;
	const wchar_t *(*paint_action)(size_t index);
};

struct feed_line {
	struct string *name; // what is displayed in menu
	struct string *link; // url of the feed
	int64_t unread_count;
};

struct item_line {
	struct string *title;
	int rowid;            // id of row in sqlite table related to this item
	bool is_unread;
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

struct config_data {
	size_t max_items;
	size_t download_timeout;
	bool append_links;
	bool run_cleaning_of_the_database_on_startup;
	bool run_analysis_of_the_database_on_startup;
	bool send_useragent_header;
	bool send_if_none_match_header;
	bool send_if_modified_since_header;
	bool ssl_verify_host;
	bool ssl_verify_peer;
	struct wstring *menu_section_entry_format;
	struct wstring *menu_feed_entry_format;
	struct wstring *menu_item_entry_format;
	struct string *global_section_name;
	struct string *contents_meta_data;
	struct string *contents_date_format;
	struct string *useragent;
	struct string *proxy;
	struct string *proxy_auth;
	size_t size_conversion_threshold;
};

typedef uint8_t input_cmd_id;
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
	INPUT_SECTIONS_MENU,
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
	FEED_COLUMN_UPDATE_DATE,
	FEED_COLUMN_DOWNLOAD_DATE,
	FEED_COLUMN_ETAG_HEADER,
	FEED_COLUMN_LAST_MODIFIED_HEADER,
	FEED_COLUMN_NONE,
};

enum item_column {
	ITEM_COLUMN_FEED_URL,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_LINK,
	ITEM_COLUMN_ATTACHMENTS,
	ITEM_COLUMN_AUTHORS,
	ITEM_COLUMN_CATEGORIES,
	ITEM_COLUMN_PUBDATE,
	ITEM_COLUMN_UPDDATE,
	ITEM_COLUMN_COMMENTS_URL,
	ITEM_COLUMN_SUMMARY,
	ITEM_COLUMN_CONTENT,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_NONE,
};

// sections
bool create_global_section(void);
bool add_feed_to_section(struct feed_line *feed, const struct string *section_name);
void obtain_feeds_of_global_section(struct feed_line ***feeds_ptr, size_t *feeds_count_ptr);
void free_sections(void);
input_cmd_id enter_sections_menu_loop(struct feed_line ***feeds_ptr, size_t *feeds_count_ptr);

// list interface
bool adjust_list_menu(void);
WINDOW *get_list_entry_by_index(size_t i);
void free_list_menu(void);
bool adjust_list_menu_format_buffer(void);
void free_list_menu_format_buffer(void);
void expose_entry_of_the_menu_list(struct menu_list_settings *settings, size_t index);
void redraw_menu_list(struct menu_list_settings *settings);
void list_menu_select_next(struct menu_list_settings *s);
void list_menu_select_prev(struct menu_list_settings *s);
void list_menu_select_next_page(struct menu_list_settings *s);
void list_menu_select_prev_page(struct menu_list_settings *s);
void list_menu_select_first(struct menu_list_settings *s);
void list_menu_select_last(struct menu_list_settings *s);

// format
const wchar_t *do_format(const struct wstring *fmt, const struct format_arg *args, size_t args_count);

// feeds
void enter_feeds_menu_loop(void);
bool load_feeds(void);
bool check_url_for_validity(const struct string *str);

// items
input_cmd_id enter_items_menu_loop(const struct string *url);

// contents
int pager_view(const struct render_block *first_block);
bool join_render_block(struct render_block **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len);
bool join_render_separator(struct render_block **list);
bool join_render_blocks_of_item_data(struct render_block **data_list, sqlite3_stmt *res);
void free_render_blocks(struct render_block *first_block);
bool populate_link_list_with_links_of_item(struct link_list *links, sqlite3_stmt *res);
bool complete_urls_of_links(struct link_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct link_list *links);
bool add_another_url_to_trim_link_list(struct link_list *links, const char *url, size_t url_len);
void free_trim_link_list(const struct link_list *links);
bool join_links_render_block(struct render_block **contents, struct link_list *links);

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

// date parsing
struct string *get_config_date_str(time_t time_ptr);

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
int64_t get_unread_items_count_of_the_feed(const struct string *url);
struct string *db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len);

bool curses_init(void);
bool obtain_terminal_size(void);

// Functions responsible for handling input.
// See "interface-input.c" file for implementation.
int get_input_command(void);
bool assign_action_to_key(int bind_key, enum input_cmd bind_cmd);
void free_binds(void);

// Functions related to window which displays status messages.
// See "interface-status.c" file for implementation.
bool status_create(void);
void status_update(void);
void status_write(const char *format, ...);
void status_clean(void);
void status_resize(void);
void status_delete(void);

// string
struct string *crtas(const char *src_ptr, size_t src_len);
struct string *crtss(const struct string *src);
struct string *crtes(void);
bool cpyas(struct string *dest, const char *src_ptr, size_t src_len);
bool cpyss(struct string *dest, const struct string *src);
bool catas(struct string *dest, const char *src_ptr, size_t src_len);
bool catss(struct string *dest, const struct string *src);
bool catcs(struct string *dest, char c);
bool string_vprintf(struct string *dest, const char *format, va_list args);
bool string_printf(struct string *dest, const char *format, ...);
void empty_string(struct string *str);
void free_string(struct string *str);
size_t convert_string_to_size_t_or_zero(const char *src);
void remove_trailing_slash_from_string(struct string *str);
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

// Parse config file, fill out config_data structure, bind keys to actions.
// See "load_config" directory for implementation.
bool load_config(void);
void free_config(void);

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
