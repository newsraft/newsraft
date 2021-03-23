#include <ncurses.h>
#define MAXPATH 512

struct string {
	char *ptr;
	size_t len;
};

struct feed_entry {
	char *name;
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
