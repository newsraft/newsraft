#ifndef NEWSRAFT_H
#define NEWSRAFT_H
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <wchar.h>
#include <pthread.h>
#include <sqlite3.h>

#ifndef NEWSRAFT_VERSION
#define NEWSRAFT_VERSION "custom"
#endif

#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWHITESPACEEXCEPTNEWLINE(A) (((A)==' ')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWIDEWHITESPACE(A) (((A)==L' ')||((A)==L'\n')||((A)==L'\t')||((A)==L'\v')||((A)==L'\f')||((A)==L'\r'))
#define INFO(...) do { if (log_stream) { fputs("[INFO] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define WARN(...) do { if (log_stream) { fputs("[WARN] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define FAIL(...) do { if (log_stream) { fputs("[FAIL] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define good_status(...) status_write(CFG_COLOR_STATUS_GOOD_FG, __VA_ARGS__)
#define info_status(...) status_write(CFG_COLOR_STATUS_INFO_FG, __VA_ARGS__)
#define fail_status(...) status_write(CFG_COLOR_STATUS_FAIL_FG, __VA_ARGS__)
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define NEWSRAFT_COLOR_PAIRS_COUNT 10
typedef uint8_t config_entry_id;
enum {
	CFG_COLOR_STATUS_GOOD_FG,
	CFG_COLOR_STATUS_GOOD_BG,
	CFG_COLOR_STATUS_INFO_FG,
	CFG_COLOR_STATUS_INFO_BG,
	CFG_COLOR_STATUS_FAIL_FG,
	CFG_COLOR_STATUS_FAIL_BG,
	CFG_COLOR_LIST_ITEM_FG,
	CFG_COLOR_LIST_ITEM_BG,
	CFG_COLOR_LIST_ITEM_UNREAD_FG,
	CFG_COLOR_LIST_ITEM_UNREAD_BG,
	CFG_COLOR_LIST_ITEM_IMPORTANT_FG,
	CFG_COLOR_LIST_ITEM_IMPORTANT_BG,
	CFG_COLOR_LIST_FEED_FG,
	CFG_COLOR_LIST_FEED_BG,
	CFG_COLOR_LIST_FEED_UNREAD_FG,
	CFG_COLOR_LIST_FEED_UNREAD_BG,
	CFG_COLOR_LIST_SECTION_FG,
	CFG_COLOR_LIST_SECTION_BG,
	CFG_COLOR_LIST_SECTION_UNREAD_FG,
	CFG_COLOR_LIST_SECTION_UNREAD_BG,
	CFG_ITEMS_COUNT_LIMIT,
	CFG_DOWNLOAD_TIMEOUT,
	CFG_DOWNLOAD_SPEED_LIMIT,
	CFG_UPDATE_THREADS_COUNT,
	CFG_SCROLLOFF,
	CFG_STATUS_MESSAGES_COUNT_LIMIT,
	CFG_OPEN_IN_BROWSER_COMMAND,
	CFG_COPY_TO_CLIPBOARD_COMMAND,
	CFG_PROXY,
	CFG_PROXY_USER,
	CFG_PROXY_PASSWORD,
	CFG_GLOBAL_SECTION_NAME,
	CFG_USER_AGENT,
	CFG_ITEM_FORMATION_ORDER,
	CFG_CONTENT_DATE_FORMAT,
	CFG_LIST_ENTRY_DATE_FORMAT,
	CFG_MENU_SECTION_ENTRY_FORMAT,
	CFG_MENU_FEED_ENTRY_FORMAT,
	CFG_MENU_ITEM_ENTRY_FORMAT,
	CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT,
	CFG_AUTO_ADVANCE_ON_READ,
	CFG_AUTO_ADVANCE_ON_UNREAD,
	CFG_MARK_ITEM_READ_ON_HOVER,
	CFG_ANALYZE_DATABASE_ON_STARTUP,
	CFG_CLEAN_DATABASE_ON_STARTUP,
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
enum {
	INPUT_SELECT_NEXT = 0,
	INPUT_SELECT_PREV,
	INPUT_SELECT_NEXT_UNREAD,
	INPUT_SELECT_PREV_UNREAD,
	INPUT_SELECT_NEXT_IMPORTANT,
	INPUT_SELECT_PREV_IMPORTANT,
	INPUT_SELECT_NEXT_PAGE,
	INPUT_SELECT_PREV_PAGE,
	INPUT_SELECT_FIRST,
	INPUT_SELECT_LAST,
	INPUT_ENTER,
	INPUT_RELOAD,
	INPUT_RELOAD_ALL,
	INPUT_MARK_READ,
	INPUT_MARK_UNREAD,
	INPUT_MARK_READ_ALL,
	INPUT_MARK_UNREAD_ALL,
	INPUT_MARK_IMPORTANT,
	INPUT_MARK_UNIMPORTANT,
	INPUT_EXPLORE_MENU,
	INPUT_STATUS_HISTORY_MENU,
	INPUT_OPEN_IN_BROWSER,
	INPUT_COPY_TO_CLIPBOARD,
	INPUT_QUIT_SOFT,
	INPUT_QUIT_HARD,
	INPUT_RESIZE,
	INPUT_ERROR,
	INPUT_SYSTEM_COMMAND,
};

typedef int8_t feeds_column_id;
enum feed_column {
	FEED_COLUMN_FEED_URL,
	FEED_COLUMN_TITLE,
	FEED_COLUMN_LINK,
	FEED_COLUMN_CONTENT,
	FEED_COLUMN_ATTACHMENTS,
	FEED_COLUMN_PERSONS,
	FEED_COLUMN_EXTRAS,
	FEED_COLUMN_DOWNLOAD_DATE,
	FEED_COLUMN_UPDATE_DATE,
	FEED_COLUMN_TIME_TO_LIVE,
	FEED_COLUMN_HTTP_HEADER_ETAG,
	FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED,
	FEED_COLUMN_HTTP_HEADER_EXPIRES,
	FEED_COLUMN_NONE,
};

typedef int8_t items_column_id;
enum {
	ITEM_COLUMN_FEED_URL,
	ITEM_COLUMN_GUID,
	ITEM_COLUMN_TITLE,
	ITEM_COLUMN_LINK,
	ITEM_COLUMN_CONTENT,
	ITEM_COLUMN_ATTACHMENTS,
	ITEM_COLUMN_PERSONS,
	ITEM_COLUMN_EXTRAS,
	ITEM_COLUMN_PUBLICATION_DATE,
	ITEM_COLUMN_UPDATE_DATE,
	ITEM_COLUMN_UNREAD,
	ITEM_COLUMN_IMPORTANT,
	ITEM_COLUMN_NONE,
};

enum {
	SECTIONS_MENU,
	FEEDS_MENU,
	ITEMS_MENU,
	MENUS_COUNT
};

typedef uint8_t sorting_order;
enum {
	SORT_BY_NONE,
	SORT_BY_TIME_DESC,
	SORT_BY_TIME_ASC,
	SORT_BY_NAME_DESC,
	SORT_BY_NAME_ASC,
};

typedef uint8_t render_block_format;
enum {
	TEXT_PLAIN,
	TEXT_RAW, // Same thing as TEXT_PLAIN, but without search for links.
	TEXT_HTML,
	TEXT_SEPARATOR,
};

typedef uint8_t format_hint_mask;
enum {
	FORMAT_BOLD_BEGIN = 1,
	FORMAT_BOLD_END = 2,
	FORMAT_ITALIC_BEGIN = 4,
	FORMAT_ITALIC_END = 8,
	FORMAT_UNDERLINED_BEGIN = 16,
	FORMAT_UNDERLINED_END = 32,
	FORMAT_ALL_END = 42, // Sum of all ending hints.
};

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

struct feed_entry {
	struct string *name;
	struct string *link;
	int64_t unread_count;
	volatile bool *volatile did_update_just_finished;
};

struct item_entry {
	struct string *title;
	struct string *url;
	const struct feed_entry *feed;
	int64_t rowid;
	bool is_unread;
	bool is_important;
	struct string *date_str;
};

struct items_list {
	struct item_entry *ptr;
	size_t len;
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

struct render_block {
	struct wstring *content;
	render_block_format content_type;
};

struct format_hint {
	format_hint_mask value;
	size_t pos;
};

struct render_blocks_list {
	struct render_block *ptr;
	size_t len;
	struct format_hint *hints;
	size_t hints_len;
};

struct link {
	struct string *url;      // URL link to data.
	struct string *type;     // Standard MIME type of data.
	struct string *size;     // Size of data in bytes.
	struct string *duration; // Duration of data in seconds.
};

struct links_list {
	struct link *ptr;
	size_t len;
};

struct deserialize_stream;

// See "sections.c" file for implementation.
bool create_global_section(void);
bool copy_feed_to_section(const struct feed_entry *feed, const struct string *section_name);
void name_feeds_by_their_titles_in_db(void);
void refresh_unread_items_count_of_all_sections(void);
void enter_sections_menu_loop(void);
void free_sections(void);
const struct format_arg *prepare_section_entry_args(size_t index);
int paint_section_entry(size_t index);
bool unread_section_condition(size_t index);

// See "feeds-parse.c" file for implementation.
bool parse_feeds_file(void);

// See "feeds.c" file for implementation.
input_cmd_id enter_feeds_menu_loop(struct feed_entry **new_feeds, size_t new_feeds_count);
const struct format_arg *prepare_feed_entry_args(size_t index);
int paint_feed_entry(size_t index);
bool unread_feed_condition(size_t index);

// See "interface-list.c" file for implementation.
bool adjust_list_menu(void);
void free_list_menu(void);
void initialize_settings_of_list_menus(void);
void expose_entry_of_the_list_menu(size_t index);
void expose_all_visible_entries_of_the_list_menu(void);
void redraw_list_menu_unprotected(void);
const size_t *enter_list_menu(int8_t menu_index, size_t new_entries_count, config_entry_id format_id);
void leave_list_menu(void);
void pause_list_menu(void);
void resume_list_menu(void);
bool handle_list_menu_navigation(input_cmd_id cmd);

// See "format.c" file for implementation.
bool create_format_buffers(void);
void free_format_buffers(void);
const wchar_t *do_format(const struct wstring *fmt, const struct format_arg *args);

// items
struct items_list *generate_items_list(struct feed_entry **feeds, size_t feeds_count, sorting_order order);
void free_items_list(struct items_list *items);
input_cmd_id enter_items_menu_loop(struct feed_entry **feeds, size_t feeds_count, config_entry_id format_id);
const struct format_arg *prepare_item_entry_args(size_t index);
int paint_item_entry(size_t index);
bool unread_item_condition(size_t index);
bool important_item_condition(size_t index);
void mark_selected_item_read(size_t view_sel);

// Functions responsible for managing render blocks.
// Render block is a piece of text in a single format. A list of render blocks
// is passed to render_data function which processes them based on their types
// and generates a single plain text buffer for a pager to display.
// See "render-block.c" file for implementation.
bool join_render_block(struct render_blocks_list *blocks, const char *content, size_t content_len, render_block_format content_type);
bool join_render_separator(struct render_blocks_list *blocks);
void free_render_blocks(struct render_blocks_list *blocks);

// Here we append links of HTML elements like <img> or <a> to link_list.
// Also, do some screen-independent processing of data that render blocks have
// (for example expand inline HTML elements like <sup>, <span> or <q>).
// See "prepare_to_render_data" directory for implementation.
bool prepare_to_render_data(struct render_blocks_list *blocks, struct links_list *links);

// Convert render blocks to one big string that can be written to pad window
// without additional splitting into lines or any other processing.
// See "render_data" directory for implementation.
struct wstring *render_data(struct render_blocks_list *blocks);

// See "items-metadata.c" file for implementation.
bool generate_render_blocks_based_on_item_data(struct render_blocks_list *blocks, sqlite3_stmt *res);

// See "items-metadata-content.c" file for implementation.
bool get_largest_piece_from_item_content(const char *content, struct string *text, render_block_format *type);
bool get_largest_piece_from_item_attachments(const char *attachments, struct string *text, render_block_format *type);

// See "items-metadata-links.c" file for implementation.
int64_t add_another_url_to_trim_links_list(struct links_list *links, const char *url, size_t url_len);
bool populate_link_list_with_links_of_item(struct links_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct links_list *links);
bool complete_urls_of_links(struct links_list *links, const struct string *feed_url);
void free_trim_link_list(const struct links_list *links);

// See "items-metadata-persons.c" file for implementation.
struct string *deserialize_persons_string(const char *src, const char *person_type);

// See "interface-pager.c" file for implementation.
int pager_view(struct render_blocks_list *blocks, bool (*custom_input_handler)(void *, input_cmd_id, uint32_t, const struct wstring *), void *data);

// See "interface-pager-item.c" file for implementation.
int enter_item_pager_view_loop(const struct item_entry *item);

// See "interface-pager-status.c" file for implementation.
int enter_status_pager_view_loop(void);

// See "threading.c" file for implementation.
bool initialize_update_threads(void);
void branch_update_feed_action_into_thread(void *(*action)(void *arg), struct feed_entry *feed);
void wait_for_all_threads_to_finish(void);
void terminate_update_threads(void);

// See "path.c" file for implementation.
bool set_feeds_path(const char *path);
bool set_config_path(const char *path);
bool set_db_path(const char *path);
const char *get_feeds_path(void);
const char *get_config_path(void);
const char *get_db_path(void);

// See "dates.c" file for implementation.
bool get_local_offset_relative_to_utc(void);
int64_t parse_date_rfc822(const struct string *value);
int64_t parse_date_rfc3339(const char *src, size_t src_len);
struct string *get_config_date_str(int64_t date, config_entry_id format_index);

// See "db.c" file for implementation.
bool db_init(void);
bool start_database_file_optimization(void);
bool catch_database_file_optimization(void);
void db_stop(void);
sqlite3_stmt *db_prepare(const char *zSql, int nByte);
bool db_begin_transaction(void);
bool db_commit_transaction(void);
bool db_rollback_transaction(void);
const char *db_error_string(void);
int db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str);
int64_t db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len);
struct string *db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len);

// See "db-items.c" file for implementation.
sqlite3_stmt *db_find_item_by_rowid(int64_t rowid);
bool db_mark_item_read(int64_t rowid);
bool db_mark_item_unread(int64_t rowid);
bool db_mark_item_important(int64_t rowid);
bool db_mark_item_unimportant(int64_t rowid);
int64_t get_unread_items_count_of_the_feed(const struct string *url);
bool db_mark_all_items_in_feeds_as_read(struct feed_entry **feeds, size_t feeds_count);
bool db_mark_all_items_in_feeds_as_unread(struct feed_entry **feeds, size_t feeds_count);

// See "interface.c" file for implementation.
bool curses_init(void);
input_cmd_id resize_counter_action(void);

// Functions responsible for curses color pairs.
// See "interface-colors.c" file for implementation.
bool create_color_pairs(void);
int get_color_pair(config_entry_id id);
int get_reversed_color_pair(config_entry_id id);

// Functions responsible for handling input and bindings.
// See "input.c" file for implementation.
input_cmd_id get_input_command(uint32_t *count, const struct wstring **macro_ptr);
void tell_program_to_terminate_safely_and_quickly(int dummy);
bool assign_action_to_key(const char *bind_key, size_t bind_key_len, input_cmd_id bind_cmd);
bool create_macro(const char *bind_key, size_t bind_key_len, const char *cmd, size_t cmd_len);
void delete_action_from_key(const char *bind_key);
void free_binds(void);
bool assign_default_binds(void);

// Functions related to window which displays status messages.
// See "interface-status.c" file for implementation.
bool status_recreate(void);
void status_clean_unprotected(void);
void status_clean(void);
void prevent_status_cleaning(void);
void allow_status_cleaning(void);
void status_write(config_entry_id color, const char *format, ...);
void status_delete(void);
struct string *generate_string_with_status_messages_for_pager(void);

// Functions related to window which displays command counter.
// See "interface-counter.c" file for implementation.
bool counter_recreate(void);
int read_counted_key_from_counter_window(uint32_t *count);
void counter_delete(void);

// Functions related to executing system commands.
// See "commands.c" file for implementation.
bool open_url_in_browser(const struct string *src);
bool copy_string_to_clipboard(const struct string *src);
bool execute_command_with_specifiers_in_it(const struct wstring *wcmd_fmt, const struct format_arg *args);

// See "string.c" file for implementation.
struct string *crtes(size_t desired_capacity);
struct string *crtas(const char *src_ptr, size_t src_len);
struct string *crtss(const struct string *src);
bool cpyas(struct string *dest, const char *src_ptr, size_t src_len);
bool cpyss(struct string *dest, const struct string *src);
bool catas(struct string *dest, const char *src_ptr, size_t src_len);
bool catss(struct string *dest, const struct string *src);
bool catcs(struct string *dest, char c);
bool crtas_or_cpyas(struct string **dest, const char *src_ptr, size_t src_len);
bool crtss_or_cpyss(struct string **dest, const struct string *src);
bool string_vprintf(struct string *dest, const char *format, va_list args);
bool string_printf(struct string *dest, const char *format, ...);
void empty_string(struct string *dest);
void free_string(struct string *str);
void trim_whitespace_from_string(struct string *str);
struct wstring *convert_string_to_wstring(const struct string *src);
struct wstring *convert_array_to_wstring(const char *src_ptr, size_t src_len);
void inlinefy_string(struct string *title);

// See "string-serialize.c" file for implementation.
bool serialize_caret(struct string **target);
bool serialize_array(struct string **target, const char *key, size_t key_len, const char *value, size_t value_len);
bool serialize_string(struct string **target, const char *key, size_t key_len, struct string *value);
struct deserialize_stream *open_deserialize_stream(const char *serialized_data);
const struct string *get_next_entry_from_deserialize_stream(struct deserialize_stream *stream);
void close_deserialize_stream(struct deserialize_stream *stream);

// See "wstring.c" file for implementation.
struct wstring *wcrtes(size_t desired_capacity);
struct wstring *wcrtas(const wchar_t *src_ptr, size_t src_len);
bool wcpyas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
bool wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
bool wcatss(struct wstring *dest, const struct wstring *src);
bool wcatcs(struct wstring *dest, wchar_t c);
bool increase_wstring_size(struct wstring *dest, size_t expansion);
void empty_wstring(struct wstring *dest);
void free_wstring(struct wstring *wstr);
void trim_whitespace_from_wstring(struct wstring *wstr);
struct string *convert_wstring_to_string(const struct wstring *src);
struct string *convert_warray_to_string(const wchar_t *src_ptr, size_t src_len);

// See "signal.c" file for implementation.
bool register_signal_handlers(void);

// Functions for opening and closing the log stream.
// To write to the log stream use macros INFO, WARN or FAIL.
// See "log.c" file for implementation.
bool log_init(const char *path);
void log_stop(int error_code);

// Parse config file, fill out config_data structure, bind keys to actions.
// See "load_config" directory for implementation.
bool load_config(void);
bool get_cfg_bool(config_entry_id i);
size_t get_cfg_uint(config_entry_id i);
int get_cfg_color(config_entry_id i);
const struct string *get_cfg_string(config_entry_id i);
const struct wstring *get_cfg_wstring(config_entry_id i);
void free_config(void);

// Download, process and store new items of feed.
// See "update_feed" directory for implementation.
void update_feeds(struct feed_entry **feeds, size_t feeds_count);

extern FILE *log_stream;
extern size_t list_menu_height;
extern size_t list_menu_width;
extern pthread_mutex_t interface_lock;
#endif // NEWSRAFT_H
