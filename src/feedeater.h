#define MAXPATH 256
int load_feeds(void);
void menu_feeds(void);
void show_feeds(void);
void close_feeds(void);
char *get_config_file_path(char *file_name);

// if cur_char is equal to one of the elements from list, then
// set position indicator of file to next character that mismatched all elements in list
void skip_chars(FILE *file, char *cur_char, char *list);
