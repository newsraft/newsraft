#include <ncurses.h>
#include <expat.h>
#define MAXPATH 512
#ifdef XML_LARGE_SIZE
#  define XML_FMT_INT_MOD "ll"
#else
#  define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
#  include <wchar.h>
#  define XML_FMT_STR "ls"
#else
#  define XML_FMT_STR "s"
#endif

struct string {
	char *ptr;
	size_t len;
};

struct feed_entry {
	char *lname; // local name of feed
	char *rname; // remote name of feed
	char *lurl;  // local url of feed
	char *rurl;  // remote url of feed
};
struct feed_window {
	struct feed_entry *feed;
	WINDOW *window;
};

struct init_parser_data {
	int depth;
	XML_Parser *xml_parser;
	struct feed_entry *(*parser_func)(XML_Parser *parser, struct string *buf);
};

enum xml_pos {
	IN_ROOT = 0,
	IN_CHANNEL_ELEMENT = 1,
	IN_ITEM_ELEMENT = 2,
	IN_IMAGE_ELEMENT = 4,
	IN_CLOUD_ELEMENT = 8,
	IN_TTL_ELEMENT = 16,
};

struct feed_parser_data {
	int depth;
	struct feed_entry *feed;
	enum xml_pos pos;
};


int load_feed_list(void);  // load feeds information in memory
void free_feed_list(void); // unload all feeds information (call this before exitting)
void show_feeds(void);    // display feeds in an interactive list
void hide_feeds(void);    // hide interactive list of feeds
void menu_feeds(void);
char *get_config_file_path(char *file_name);
char *get_data_dir_for_url(char *url);

void free_feed_entry(struct feed_entry *);

// try to reload feeds
void feed_reload(struct feed_entry *);
void feed_reload_all(void);

struct string *feed_download(char *url);

// return most sensible string for feed title
char *feed_image(struct feed_entry *feed);

// try to list items of the feed url
void feed_view(char *url);

// if cur_char is equal to one of the elements from list, then
// set position indicator of file to next character that mismatched all elements in list
void skip_chars(FILE *file, char *cur_char, char *list);

int status_create(void);
void status_write(char *format, ...);
int status_delete(void);

struct feed_entry *parse_rss20(XML_Parser *parser, struct string *buf);  // RSS 2.0
struct feed_entry *parse_rss11(XML_Parser *parser, struct string *buf);  // RSS 1.1
struct feed_entry *parse_rss10(XML_Parser *parser, struct string *buf);  // RSS 1.0
struct feed_entry *parse_rss094(XML_Parser *parser, struct string *buf); // RSS 0.94
struct feed_entry *parse_rss092(XML_Parser *parser, struct string *buf); // RSS 0.92
struct feed_entry *parse_rss091(XML_Parser *parser, struct string *buf); // RSS 0.91
struct feed_entry *parse_rss090(XML_Parser *parser, struct string *buf); // RSS 0.90
struct feed_entry *parse_atom10(XML_Parser *parser, struct string *buf); // Atom 1.0
struct feed_entry *parse_atom03(XML_Parser *parser, struct string *buf); // Atom 0.3
struct feed_entry *parse_json11(XML_Parser *parser, struct string *buf); // JSON 1.1

