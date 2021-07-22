#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int64_t config_max_items = 100;
size_t config_init_parser_buf_size = 100000;

bool config_menu_show_number = true;

char *config_contents_meta_data = "feed,title,author,category,date,url,comments";
char *config_contents_date_format = "%a, %d %b %Y %H:%M:%S %z";

char config_key_mark_marked = 'b';
char config_key_mark_unmarked = 'B';
char config_key_mark_read = 'r';
char config_key_mark_unread = 'R';
char config_key_mark_read_all = 'a';
char config_key_mark_unread_all = 'A';
char config_key_download = 'd';
char config_key_download_all = 'D';
char config_key_soft_quit = 'q';
char config_key_hard_quit = 'Q';
