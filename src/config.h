#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <stdbool.h>

extern int64_t config_max_items;
extern size_t config_init_parser_buf_size;

extern bool config_menu_show_number;
extern bool config_menu_show_decoration_number;

extern char *config_contents_meta_data;
extern char *config_contents_date_format;

extern char config_key_mark_marked;
extern char config_key_mark_unmarked;
extern char config_key_mark_read;
extern char config_key_mark_read_all;
extern char config_key_mark_unread;
extern char config_key_mark_unread_all;
extern char config_key_download;
extern char config_key_download_all;
extern char config_key_soft_quit;
extern char config_key_hard_quit;
#endif // CONFIG_H
