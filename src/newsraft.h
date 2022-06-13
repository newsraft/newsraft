#ifndef NEWSRAFT_H
#define NEWSRAFT_H
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>
#include <wchar.h>
#include <ncurses.h>
#include <sqlite3.h>

#ifndef NEWSRAFT_VERSION
#define NEWSRAFT_VERSION "custom"
#endif

#define COUNTOF(A) (sizeof(A) / sizeof(*A))
#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWHITESPACEEXCEPTNEWLINE(A) (((A)==' ')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWIDEWHITESPACE(A) (((A)==L' ')||((A)==L'\n')||((A)==L'\t')||((A)==L'\v')||((A)==L'\f')||((A)==L'\r'))
#define INFO(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[INFO] " A "\n", ##__VA_ARGS__); } } while (0)
#define WARN(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[WARN] " A "\n", ##__VA_ARGS__); } } while (0)
#define FAIL(A, ...) do { if (log_stream != NULL) { fprintf(log_stream, "[FAIL] " A "\n", ##__VA_ARGS__); } } while (0)

// Limits that will most likely never be reached, but are needed to avoid
// unexpected crashes.
#define MIME_TYPE_LENGTH_LIMIT 255
#define FORMAT_STRING_LENGTH_LIMIT 1000

struct string {
	char *ptr;
	size_t len;
	size_t lim;
};

struct string_list {
	struct string *str;
	struct string_list *next;
};

struct string_deserialize_stream;

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

struct item_entry {
	struct string *title;
	const struct string *feed_name; // name of the feed to which this item belongs
	int rowid;                      // id of row in sqlite table related to this item
	bool is_unread;
	struct string *date_str;
};

struct items_list {
	struct item_entry *list;
	size_t count;
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
};

typedef uint8_t config_entry_id;
enum config_entry_index {
	CFG_ITEMS_COUNT_LIMIT,
	CFG_DOWNLOAD_TIMEOUT,
	CFG_DOWNLOAD_SPEED_LIMIT,
	CFG_STATUS_MESSAGES_LIMIT,
	CFG_SIZE_CONVERSION_THRESHOLD,
	CFG_COPY_TO_CLIPBOARD_COMMAND,
	CFG_PROXY,
	CFG_PROXY_AUTH,
	CFG_GLOBAL_SECTION_NAME,
	CFG_USER_AGENT,
	CFG_CONTENT_DATA_ORDER,
	CFG_CONTENT_DATE_FORMAT,
	CFG_LIST_ENTRY_DATE_FORMAT,
	CFG_MENU_SECTION_ENTRY_FORMAT,
	CFG_MENU_FEED_ENTRY_FORMAT,
	CFG_MENU_ITEM_ENTRY_FORMAT,
	CFG_MENU_OVERVIEW_ITEM_ENTRY_FORMAT,
	CFG_MARK_ITEM_READ_ON_HOVER,
	CFG_CONTENT_APPEND_LINKS,
	CFG_CLEAN_DATABASE_ON_STARTUP,
	CFG_ANALYZE_DATABASE_ON_STARTUP,
	CFG_RESPECT_TTL_ELEMENT,
	CFG_RESPECT_EXPIRES_HEADER,
	CFG_SEND_USER_AGENT_HEADER,
	CFG_SEND_IF_NONE_MATCH_HEADER,
	CFG_SEND_IF_MODIFIED_SINCE_HEADER,
	CFG_SSL_VERIFY_HOST,
	CFG_SSL_VERIFY_PEER,
	CFG_ENTRIES_COUNT,
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
	INPUT_OVERVIEW_MENU,
	INPUT_SECTIONS_MENU,
	INPUT_STATUS_HISTORY_MENU,
	INPUT_COPY_TO_CLIPBOARD,
	INPUT_RESIZE,
	INPUTS_COUNT,
};

enum feed_column {
	FEED_COLUMN_FEED_URL,
	FEED_COLUMN_GUID,
	FEED_COLUMN_TITLE,
	FEED_COLUMN_LINK,
	FEED_COLUMN_SUMMARY,
	FEED_COLUMN_AUTHORS,
	FEED_COLUMN_CATEGORIES,
	FEED_COLUMN_LANGUAGES,
	FEED_COLUMN_RIGHTS,
	FEED_COLUMN_RATING,
	FEED_COLUMN_PICTURES,
	FEED_COLUMN_GENERATORS,
	FEED_COLUMN_DOWNLOAD_DATE,
	FEED_COLUMN_UPDATE_DATE,
	FEED_COLUMN_TIME_TO_LIVE,
	FEED_COLUMN_HTTP_HEADER_ETAG,
	FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED,
	FEED_COLUMN_HTTP_HEADER_EXPIRES,
	FEED_COLUMN_NONE,
};

enum item_column {
	ITEM_COLUMN_FEED_URL,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_LINK,
	ITEM_COLUMN_SUMMARY,
	ITEM_COLUMN_CONTENT,
	ITEM_COLUMN_ATTACHMENTS,
	ITEM_COLUMN_SOURCES,
	ITEM_COLUMN_AUTHORS,
	ITEM_COLUMN_COMMENTS_URL,
	ITEM_COLUMN_LOCATIONS,
	ITEM_COLUMN_CATEGORIES,
	ITEM_COLUMN_LANGUAGES,
	ITEM_COLUMN_RIGHTS,
	ITEM_COLUMN_RATING,
	ITEM_COLUMN_PICTURES,
	ITEM_COLUMN_PUBLICATION_DATE,
	ITEM_COLUMN_UPDATE_DATE,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_NONE,
};

enum sorting_order {
	SORT_BY_NONE,
	SORT_BY_TIME_DESC,
	SORT_BY_TIME_ASC,
	SORT_BY_NAME_DESC,
	SORT_BY_NAME_ASC,
};

// sections
bool create_global_section(void);
bool copy_feed_to_section(const struct feed_line *feed, const struct string *section_name);
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
void expose_all_visible_entries_of_the_menu_list(struct menu_list_settings *settings);
void redraw_menu_list(struct menu_list_settings *settings);
void list_menu_select_next(struct menu_list_settings *s);
void list_menu_select_prev(struct menu_list_settings *s);
void list_menu_select_next_page(struct menu_list_settings *s);
void list_menu_select_prev_page(struct menu_list_settings *s);
void list_menu_select_first(struct menu_list_settings *s);
void list_menu_select_last(struct menu_list_settings *s);

// format
const wchar_t *do_format(int format_setting, const struct format_arg *args, size_t args_count);

// feeds
bool parse_feeds_file(const char *path);
void enter_feeds_menu_loop(void);
bool load_feeds(void);
bool check_url_for_validity(const struct string *str);

// items
struct items_list *generate_items_list(const struct feed_line **feeds, size_t feeds_count, enum sorting_order order);
void free_items_list(struct items_list *items);
input_cmd_id enter_items_menu_loop(const struct feed_line **feeds, size_t feeds_count, int format);

// Functions responsible for managing render blocks.
// Render block is a piece of text in a single format. They are stored as linked
// list and sent to pager_view function so it can generate a single text buffer
// out of texts with different types (plain, html, markdown).
// See "render-block.c" file for implementation.
bool join_render_block(struct render_block **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len);
void reverse_render_blocks(struct render_block **list);
bool join_render_separator(struct render_block **list);
bool join_render_blocks_of_item_data(struct render_block **data_list, sqlite3_stmt *res);
void free_render_blocks(struct render_block *first_block);

// contents
bool populate_link_list_with_links_of_item(struct link_list *links, sqlite3_stmt *res);
bool complete_urls_of_links(struct link_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct link_list *links);
int64_t add_another_url_to_trim_link_list(struct link_list *links, const char *url, size_t url_len);
void free_trim_link_list(const struct link_list *links);

// pager
int pager_view(const struct render_block *first_block, void (*custom_input_handler)(void *data, input_cmd_id cmd, uint32_t count), void *data);
int enter_item_pager_view_loop(int rowid);
int enter_status_pager_view_loop(void);

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
struct string *get_config_date_str(time_t date, enum config_entry_index format_index);
struct string *get_http_date_str(time_t date);

// db
bool db_init(void);
void db_stop(void);
bool db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt);
bool db_begin_transaction(void);
bool db_commit_transaction(void);
bool db_rollback_transaction(void);
const char *db_error_string(void);
int db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str);
struct string *db_get_plain_text_from_column(sqlite3_stmt *res, int column);
int64_t db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len);
struct string *db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len);

// db-items.c
sqlite3_stmt *db_find_item_by_rowid(int rowid);
bool db_mark_item_read(int rowid);
bool db_mark_item_unread(int rowid);
int64_t get_unread_items_count_of_the_feed(const struct string *url);
bool db_mark_all_items_in_feeds_as_read(const struct feed_line **feeds, size_t feeds_count);
bool db_mark_all_items_in_feeds_as_unread(const struct feed_line **feeds, size_t feeds_count);

// interface
bool curses_init(void);
bool resize_counter_action(void);

// Functions responsible for handling input.
// See "interface-input.c" file for implementation.
int get_input_command(uint32_t *count);
bool assign_default_binds(void);
bool assign_action_to_key(int bind_key, enum input_cmd bind_cmd);
void free_binds(void);

// Functions related to window which displays status messages.
// See "interface-status.c" file for implementation.
bool status_create(void);
void status_update(void);
void status_write(const char *format, ...);
void status_clean(void);
bool status_resize(void);
void status_delete(void);
int read_key_from_status(void);
struct string *generate_string_with_status_messages_for_pager(void);

// Functions related to window which displays command counter.
// See "interface-counter.c" file for implementation.
bool counter_create(void);
void counter_send_character(char c);
uint32_t counter_extract_count(void);
void counter_clean(void);
bool counter_resize(void);
void counter_delete(void);

// string.c
struct string *crtas(const char *src_ptr, size_t src_len);
struct string *crtss(const struct string *src);
struct string *crtes(void);
bool cpyas(struct string *dest, const char *src_ptr, size_t src_len);
bool cpyss(struct string *dest, const struct string *src);
bool catas(struct string *dest, const char *src_ptr, size_t src_len);
bool catss(struct string *dest, const struct string *src);
bool catcs(struct string *dest, char c);
bool crtas_or_cpyas(struct string **dest, const char *src_ptr, size_t src_len);
bool crtss_or_cpyss(struct string **dest, const struct string *src);
bool string_vprintf(struct string *dest, const char *format, va_list args);
bool string_printf(struct string *dest, const char *format, ...);
void empty_string(struct string *str);
void empty_string_safe(struct string *str);
void free_string(struct string *str);
size_t convert_string_to_size_t_or_zero(const char *src);
void remove_character_from_string(struct string *str, char c);
void remove_trailing_slashes_from_string(struct string *str);
void trim_whitespace_from_string(struct string *str);
struct wstring *convert_string_to_wstring(const struct string *src);
struct string *convert_bytes_to_human_readable_size_string(const char *value);

// string-serialize.c
bool cat_array_to_serialization(struct string **target, const char *key, size_t key_len, const char *value, size_t value_len);
bool cat_string_to_serialization(struct string **target, const char *key, size_t key_len, struct string *value);
struct string_deserialize_stream *open_string_deserialize_stream(const char *serialized_data);
const struct string *get_next_entry_from_deserialize_stream(struct string_deserialize_stream *stream);
void close_string_deserialize_stream(struct string_deserialize_stream *stream);

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
const char *get_cfg_name(size_t i);
bool get_cfg_bool(size_t i);
size_t get_cfg_uint(size_t i);
const struct string *get_cfg_string(size_t i);
const struct wstring *get_cfg_wstring(size_t i);
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

#endif // NEWSRAFT_H
