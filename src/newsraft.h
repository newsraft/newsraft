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
#include <curses.h>

#ifndef NEWSRAFT_VERSION
#define NEWSRAFT_VERSION "0.22"
#endif

#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWHITESPACEEXCEPTNEWLINE(A) (((A)==' ')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))
#define ISWIDEWHITESPACE(A) (((A)==L' ')||((A)==L'\n')||((A)==L'\t')||((A)==L'\v')||((A)==L'\f')||((A)==L'\r'))
#define ISDIGIT(A) (((A)=='0')||((A)=='1')||((A)=='2')||((A)=='3')||((A)=='4')||((A)=='5')||((A)=='6')||((A)=='7')||((A)=='8')||((A)=='9'))
#define INFO(...) do { if (log_stream) { fputs("[INFO] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define WARN(...) do { if (log_stream) { fputs("[WARN] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define FAIL(...) do { if (log_stream) { fputs("[FAIL] ", log_stream); fprintf(log_stream, __VA_ARGS__); fputc('\n', log_stream); } } while (0)
#define good_status(...) status_write(CFG_COLOR_STATUS_GOOD_FG, __VA_ARGS__)
#define info_status(...) status_write(CFG_COLOR_STATUS_INFO_FG, __VA_ARGS__)
#define fail_status(...) status_write(CFG_COLOR_STATUS_FAIL_FG, __VA_ARGS__)
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define NEWSRAFT_THREADS_COUNT_LIMIT 100

#define NEWSRAFT_COLOR_PAIRS_COUNT 10
typedef uint8_t config_entry_id;
enum {
	CFG_COLOR_STATUS_GOOD_FG = 0,
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
	CFG_SCROLLOFF,
	CFG_ITEMS_COUNT_LIMIT,
	CFG_UPDATE_THREADS_COUNT,
	CFG_DOWNLOAD_TIMEOUT,
	CFG_DOWNLOAD_SPEED_LIMIT,
	CFG_STATUS_MESSAGES_COUNT_LIMIT,
	CFG_COPY_TO_CLIPBOARD_COMMAND,
	CFG_PROXY,
	CFG_PROXY_USER,
	CFG_PROXY_PASSWORD,
	CFG_GLOBAL_SECTION_NAME,
	CFG_USER_AGENT,
	CFG_ITEM_CONTENT_FORMAT,
	CFG_ITEM_CONTENT_DATE_FORMAT,
	CFG_LIST_ENTRY_DATE_FORMAT,
	CFG_OPEN_IN_BROWSER_COMMAND,
	CFG_MENU_SECTION_ENTRY_FORMAT,
	CFG_MENU_FEED_ENTRY_FORMAT,
	CFG_MENU_ITEM_ENTRY_FORMAT,
	CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT,
	CFG_SECTIONS_MENU_PARAMOUNT_EXPLORE,
	CFG_FEEDS_MENU_PARAMOUNT_EXPLORE,
	CFG_INITIAL_UNREAD_FIRST_SORTING,
	CFG_MARK_ITEM_READ_ON_HOVER,
	CFG_ANALYZE_DATABASE_ON_STARTUP,
	CFG_CLEAN_DATABASE_ON_STARTUP,
	CFG_RESPECT_TTL_ELEMENT,
	CFG_RESPECT_EXPIRES_HEADER,
	CFG_SEND_USER_AGENT_HEADER,
	CFG_SEND_IF_NONE_MATCH_HEADER,
	CFG_SEND_IF_MODIFIED_SINCE_HEADER,
	CFG_ENTRIES_COUNT,
};

typedef uint8_t input_cmd_id;
enum {
	INPUT_SELECT_NEXT = 0,
	INPUT_SELECT_PREV,
	INPUT_SELECT_NEXT_PAGE,
	INPUT_SELECT_PREV_PAGE,
	INPUT_SELECT_FIRST,
	INPUT_SELECT_LAST,
	INPUT_JUMP_TO_NEXT,
	INPUT_JUMP_TO_PREV,
	INPUT_JUMP_TO_NEXT_UNREAD,
	INPUT_JUMP_TO_PREV_UNREAD,
	INPUT_JUMP_TO_NEXT_IMPORTANT,
	INPUT_JUMP_TO_PREV_IMPORTANT,
	INPUT_SORT_NEXT,
	INPUT_SORT_PREV,
	INPUT_TOGGLE_UNREAD_FIRST_SORTING,
	INPUT_ENTER,
	INPUT_RELOAD,
	INPUT_RELOAD_ALL,
	INPUT_MARK_READ,
	INPUT_MARK_UNREAD,
	INPUT_MARK_READ_ALL,
	INPUT_MARK_UNREAD_ALL,
	INPUT_MARK_IMPORTANT,
	INPUT_MARK_UNIMPORTANT,
	INPUT_TOGGLE_EXPLORE_MODE,
	INPUT_STATUS_HISTORY_MENU,
	INPUT_OPEN_IN_BROWSER,
	INPUT_COPY_TO_CLIPBOARD,
	INPUT_START_SEARCH_INPUT,
	INPUT_NAVIGATE_BACK,
	INPUT_QUIT_SOFT,
	INPUT_QUIT_HARD,
	INPUT_RESIZE,
	INPUT_SYSTEM_COMMAND,
	INPUT_ERROR,
	INPUT_APPLY_SEARCH_MODE_FILTER,
	INPUT_ITEMS_MENU_WAS_NOT_CREATED,
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
	PAGER_MENU,
	MENUS_COUNT,
};

typedef int8_t sorting_order; // Need negative numbers somewhere.
enum {
	SORT_BY_TIME_DESC = 0,
	SORT_BY_TIME_ASC,
	SORT_BY_TITLE_DESC,
	SORT_BY_TITLE_ASC,
};

typedef uint8_t render_block_format;
enum {
	TEXT_PLAIN,
	TEXT_RAW, // Same thing as TEXT_PLAIN, but without search for links.
	TEXT_HTML,
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
	int64_t download_date;
	int64_t update_period;
	volatile bool *volatile did_update_just_finished;
};

struct item_entry {
	struct string *title;
	struct string *url;
	struct feed_entry *feed;
	int64_t rowid;
	bool is_unread;
	bool is_important;
	int64_t pub_date;
	int64_t upd_date;
	struct string *date_str;
	struct string *pub_date_str;
};

struct items_list {
	sqlite3_stmt *res;
	struct string *query;
	struct string *search_filter;
	struct item_entry *ptr;
	size_t len;
	bool finished;
	sorting_order sort;
	bool show_unread_first;
	struct feed_entry **feeds; // Just a pointer to parent feeds.
	size_t feeds_count;
};

struct format_arg {
	const wchar_t specifier;
	const wchar_t type_specifier;
	union {
		int i;
		const char *s;
	} value;
};

struct render_block {
	struct wstring *content;
	render_block_format content_type;
	bool needs_trimming;
};

struct render_blocks_list {
	struct render_block *ptr;
	size_t len;
	size_t links_block_index;
	struct string *pre_links_block;
	struct string *post_links_block;
};

struct format_hint {
	format_hint_mask value;
	size_t pos;
};

struct render_line {
	struct wstring *ws;
	struct format_hint *hints;
	size_t hints_len;
};

struct render_result {
	struct render_line *lines;
	size_t lines_len;
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
int64_t make_sure_section_exists(const struct string *section_name);
bool copy_feed_to_section(const struct feed_entry *feed, int64_t section_index);
void refresh_unread_items_count_of_all_sections(void);
bool purge_abandoned_feeds(void);
void enter_sections_menu_loop(void);
void free_sections(void);
void process_auto_updating_feeds(void);
bool sections_list_moderator(size_t index);
const struct format_arg *get_section_entry_args(size_t index);
int paint_section_entry(size_t index);
bool unread_section_condition(size_t index);
void mark_section_read(size_t view_sel);
void mark_section_unread(size_t view_sel);

// See "feeds-parse.c" file for implementation.
bool parse_feeds_file(void);

// See "feeds.c" file for implementation.
void mark_feed_read(size_t view_sel);
void mark_feed_unread(size_t view_sel);
input_cmd_id enter_feeds_menu_loop(struct feed_entry **new_feeds, size_t new_feeds_count);
bool feeds_list_moderator(size_t index);
const struct format_arg *get_feed_entry_args(size_t index);
int paint_feed_entry(size_t index);
bool unread_feed_condition(size_t index);

// See "interface-list.c" file for implementation.
int8_t get_current_menu_type(void);
bool adjust_list_menu(void);
void free_list_menu(void);
void initialize_settings_of_list_menus(void);
void expose_entry_of_the_list_menu(size_t index);
void expose_all_visible_entries_of_the_list_menu(void);
void redraw_list_menu_unprotected(void);
const size_t *enter_list_menu(int8_t menu_index, config_entry_id format_id, bool do_reset);
void reset_list_menu_unprotected(void);
bool handle_list_menu_control(uint8_t menu_id, input_cmd_id cmd, const struct wstring *arg);
bool handle_pager_menu_control(input_cmd_id cmd);

// See "interface-list-pager.c" file for implementation.
bool pager_menu_moderator(size_t index);
void pager_list_menu_writer(size_t index, WINDOW *w);
bool start_pager_menu(struct render_blocks_list *new_blocks);
bool refresh_pager_menu(void);

// See "format.c" file for implementation.
void do_format(struct wstring *dest, const wchar_t *fmt, const struct format_arg *args);

// See "items.c" file for implementation.
void mark_item_read(size_t view_sel);
void mark_item_unread(size_t view_sel);
bool items_list_moderator(size_t index);
const struct format_arg *get_item_entry_args(size_t index);
int paint_item_entry(size_t index);
bool unread_item_condition(size_t index);
bool important_item_condition(size_t index);
void clean_up_items_menu(void);
void tell_items_menu_to_regenerate(void);
input_cmd_id enter_items_menu_loop(struct feed_entry **new_feeds, size_t new_feeds_count, bool is_explore_mode, const struct string *search_filter);

// See "items-list.c" file for implementation.
struct items_list *create_items_list(struct feed_entry **feeds, size_t feeds_count, sorting_order order, bool unread_first, const struct string *search_filter);
bool replace_items_list_with_empty_one(struct items_list **items);
void obtain_items_at_least_up_to_the_given_index(struct items_list *items, size_t index);
void change_sorting_order_of_items_list(struct items_list **items, sorting_order order);
void toggle_unread_first_sorting_of_items_list(struct items_list **items);
void change_search_filter_of_items_list(struct items_list **items, const struct string *search_filter);
void free_items_list(struct items_list *items);

// See "items-pager.c" file for implementation.
int enter_item_pager_view_loop(struct items_list *items, const size_t *view_sel);

// Functions responsible for managing render blocks.
// Render block is a piece of text in a single format. A list of render blocks
// is passed to render_data function which processes them based on their types
// and generates a single plain text buffer for a pager to display.
// See "render-block.c" file for implementation.
bool add_render_block(struct render_blocks_list *blocks, const char *content, size_t content_len, render_block_format content_type, bool needs_trimming);
bool apply_links_render_blocks(struct render_blocks_list *blocks, const struct string *data);
void free_render_blocks(struct render_blocks_list *blocks);

// Here we extract links from texts of render_block entries into links_list and
// insert link marks into texts so that it's more convenient for user to work
// with the list of links in the pager. Also, we do here some screen-independent
// processing of texts that render_block entries have, for example expand a few
// inline HTML elements like <span>, <sup>, <q>, etc.
// See "prepare_to_render_data" directory for implementation.
bool prepare_to_render_data(struct render_blocks_list *blocks, struct links_list *links);

// See "render_data" directory for implementation.
bool render_data(struct render_result *result, struct render_blocks_list *blocks);

// See "items-metadata.c" file for implementation.
bool generate_render_blocks_based_on_item_data(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res);

// See "items-metadata-content.c" file for implementation.
bool get_largest_piece_from_item_content(const char *content, struct string **text, render_block_format *type);
bool get_largest_piece_from_item_attachments(const char *attachments, struct string **text, render_block_format *type);

// See "items-metadata-links.c" file for implementation.
int64_t add_another_url_to_trim_links_list(struct links_list *links, const char *url, size_t url_len);
bool populate_link_list_with_links_of_item(struct links_list *links, sqlite3_stmt *res);
struct string *generate_link_list_string_for_pager(const struct links_list *links);
bool complete_urls_of_links(struct links_list *links, const struct string *feed_url);
void free_links_list(const struct links_list *links);

// See "items-metadata-persons.c" file for implementation.
struct string *deserialize_persons_string(const char *src);

// See "path.c" file for implementation.
bool set_feeds_path(const char *path);
bool set_config_path(const char *path);
bool set_db_path(const char *path);
const char *get_feeds_path(void);
const char *get_config_path(void);
const char *get_db_path(void);

// See "dates.c" file for implementation.
int64_t parse_date_rfc3339(const char *src);
struct string *get_config_date_str(int64_t date, config_entry_id format_index);

// See "db.c" file for implementation.
bool db_init(void);
bool db_vacuum(void);
bool query_database_file_optimization(void);
void db_stop(void);
sqlite3_stmt *db_prepare(const char *zSql, int nByte);
bool db_begin_transaction(void);
bool db_commit_transaction(void);
bool db_rollback_transaction(void);
const char *db_error_string(void);
int db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str);
int64_t db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len);
struct string *db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len);
void db_set_download_date(const struct string *url, int64_t download_date);

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
input_cmd_id resize_handler(void);
bool call_resize_handler_if_current_list_menu_size_is_different_from_actual(void);

// Functions related to window which reads user input and displays command
// counter (prefix number before command).
// See "interface-input.c" file for implementation.
bool counter_recreate_unprotected(void);
void tell_program_to_terminate_safely_and_quickly(int dummy);
input_cmd_id get_input_command(uint32_t *count, const struct wstring **macro_ptr);
void break_getting_input_command(void);
void counter_delete(void);

// Functions responsible for curses color pairs.
// See "interface-colors.c" file for implementation.
bool create_color_pairs(void);
unsigned int get_color_pair(config_entry_id id);

// Functions related to window which displays status messages.
// See "interface-status.c" file for implementation.
void update_status_window_content(void);
bool status_recreate_unprotected(void);
bool allocate_status_messages_buffer(void);
void status_clean_unprotected(void);
void status_clean(void);
void prevent_status_cleaning(void);
void allow_status_cleaning(void);
void status_write(config_entry_id color, const char *format, ...);
void status_delete(void);
struct string *generate_string_with_status_messages_for_pager(void);

// See "interface-status-pager.c" file for implementation.
int enter_status_pager_view_loop(void);

// Functions responsible for managing of key bindings.
// See "binds.c" file for implementation.
input_cmd_id get_action_of_bind(int key_code, size_t action_index, const struct wstring **macro_ptr);
ssize_t create_empty_bind_or_clean_existing(const char *key_name, size_t key_name_len);
bool attach_cmd_action_to_bind(ssize_t bind_index, input_cmd_id cmd_action);
bool attach_exec_action_to_bind(ssize_t bind_index, const char *exec, size_t exec_len);
bool assign_default_binds(void);
void free_binds(void);

// Functions related to executing system commands.
// See "commands.c" file for implementation.
void copy_string_to_clipboard(const struct string *src);
void run_command_with_specifiers(const struct wstring *wcmd_fmt, const struct format_arg *args);

// See "string.c" file for implementation.
struct string *crtes(size_t desired_capacity);
struct string *crtas(const char *src_ptr, size_t src_len);
struct string *crtss(const struct string *src);
bool cpyas(struct string **dest, const char *src_ptr, size_t src_len);
bool cpyss(struct string **dest, const struct string *src);
bool catas(struct string *dest, const char *src_ptr, size_t src_len);
bool catss(struct string *dest, const struct string *src);
bool catcs(struct string *dest, char c);
bool string_vprintf(struct string *dest, const char *format, va_list args);
void empty_string(struct string *dest);
void free_string(struct string *str);
void trim_whitespace_from_string(struct string *str);
struct wstring *convert_string_to_wstring(const struct string *src);
struct wstring *convert_array_to_wstring(const char *src_ptr, size_t src_len);
void inlinefy_string(struct string *title);

// See "string-serialize.c" file for implementation.
bool serialize_caret(struct string **target);
bool serialize_array(struct string **target, const char *key, size_t key_len, const char *value, size_t value_len);
bool serialize_string(struct string **target, const char *key, size_t key_len, const struct string *value);
struct deserialize_stream *open_deserialize_stream(const char *serialized_data);
const struct string *get_next_entry_from_deserialize_stream(struct deserialize_stream *stream);
void close_deserialize_stream(struct deserialize_stream *stream);

// See "wstring.c" file for implementation.
bool wstr_set(struct wstring **dest, const wchar_t *src_ptr, size_t src_len, size_t src_lim);
struct wstring *wcrtes(size_t desired_capacity);
struct wstring *wcrtas(const wchar_t *src_ptr, size_t src_len);
bool wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len);
bool wcatss(struct wstring *dest, const struct wstring *src);
bool wcatcs(struct wstring *dest, wchar_t c);
bool make_sure_there_is_enough_space_in_wstring(struct wstring *dest, size_t need_space);
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
bool parse_config_file(void);
bool load_config(void);
bool get_cfg_bool(config_entry_id i);
size_t get_cfg_uint(config_entry_id i);
int get_cfg_color(config_entry_id i, unsigned int *attribute);
const struct string *get_cfg_string(config_entry_id i);
const struct wstring *get_cfg_wstring(config_entry_id i);
void free_config(void);

// Download, process and store new items of feed.
// See "update_feed" directory for implementation.
void update_feeds(struct feed_entry **feeds, size_t feeds_count);
bool start_feed_updater(void);
void stop_feed_updater(void);

extern volatile bool they_want_us_to_terminate;
extern FILE *log_stream;
extern size_t list_menu_height;
extern size_t list_menu_width;
extern bool search_mode_is_enabled;
extern struct string *search_mode_text_input;
extern pthread_mutex_t interface_lock;
#endif // NEWSRAFT_H
