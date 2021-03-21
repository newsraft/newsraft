#include <ncurses.h>
#define MAXPATH 512
#define STATUS_MSG(FMT, ...) wclear(status_win); mvwprintw(status_win, 0, 0, FMT, ##__VA_ARGS__); wrefresh(status_win)
int load_feeds(void);   // load feeds information in memory
void show_feeds(void);  // display feeds in an interactive list
void hide_feeds(void);  // hide interactive list of feeds
void menu_feeds(void);
void close_feeds(void); // unload all feeds information (call this before exitting)
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
