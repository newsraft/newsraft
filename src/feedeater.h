#include <ncurses.h>
#define MAXPATH 512
int load_feeds(void);
void menu_feeds(void);
void show_feeds(void);
void close_feeds(void);
char *get_config_file_path(char *file_name);
char *get_data_dir_for_url(char *url);

// try to reload feeds
void feed_reload(char *url);
void feed_reload_all(void);

// try to list items of the feed url
void feed_view(char *url);

// if cur_char is equal to one of the elements from list, then
// set position indicator of file to next character that mismatched all elements in list
void skip_chars(FILE *file, char *cur_char, char *list);

extern WINDOW *status_win;
