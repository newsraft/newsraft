#define MAXPATH 256
int load_feeds(void);
int menu_feeds(void);
int feeds_rel_select(int i);
int feeds_abs_select(int i);
int show_feeds(void);
int close_feeds(void);
char *get_config_file_path(char *file_name);
