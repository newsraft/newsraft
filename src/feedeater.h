#include <ncurses.h>
#define MAXPATH 512

struct string {
	char *ptr;
	size_t len;
};

struct feed_entry {
	char *custom_name; // name of feed set by user
	char *remote_name; // name of feed set by author
	char *url;
	WINDOW *window;
};

int load_feed_list(void);  // load feeds information in memory
void free_feed_list(void); // unload all feeds information (call this before exitting)
void show_feeds(void);    // display feeds in an interactive list
void hide_feeds(void);    // hide interactive list of feeds
void menu_feeds(void);
char *get_config_file_path(char *file_name);
char *get_data_dir_for_url(char *url);

// try to reload feeds
void feed_reload(struct feed_entry *);
void feed_reload_all(void);

struct string *feed_download(char *url);

// try to list items of the feed url
void feed_view(char *url);

// if cur_char is equal to one of the elements from list, then
// set position indicator of file to next character that mismatched all elements in list
void skip_chars(FILE *file, char *cur_char, char *list);

int status_create(void);
void status_write(char *format, ...);
int status_delete(void);

struct parsing_buffer2 {
	int depth;
	struct feed_entry *feed;
};

struct feed_entry *parse_rss20(struct string *buf);  // RSS 2.0
struct feed_entry *parse_rss11(struct string *buf);  // RSS 1.1
struct feed_entry *parse_rss10(struct string *buf);  // RSS 1.0
struct feed_entry *parse_rss094(struct string *buf); // RSS 0.94
struct feed_entry *parse_rss092(struct string *buf); // RSS 0.92
struct feed_entry *parse_rss091(struct string *buf); // RSS 0.91
struct feed_entry *parse_rss090(struct string *buf); // RSS 0.90
struct feed_entry *parse_atom10(struct string *buf); // Atom 1.0
struct feed_entry *parse_atom03(struct string *buf); // Atom 0.3
struct feed_entry *parse_json11(struct string *buf); // JSON 1.1
