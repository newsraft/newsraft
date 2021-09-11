#include "feedeater.h"

/* don't initialize strings because then they will be immutable */
char *config_menu_set_entry_format = NULL;
char *config_contents_meta_data = NULL;
char *config_contents_date_format = NULL;

size_t config_max_items = 100; // 0 == inf
size_t config_init_parser_buf_size = 100000;

char config_key_mark_read = 'r';
char config_key_mark_unread = 'R';
char config_key_mark_read_all = 'a';
char config_key_mark_unread_all = 'A';
char config_key_download = 'd';
char config_key_download_all = 'D';
char config_key_soft_quit = 'q';
char config_key_hard_quit = 'Q';
