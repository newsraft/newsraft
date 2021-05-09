#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <stdbool.h>

extern int64_t config_max_items;

extern bool   config_menu_show_number;
extern bool   config_contents_show_feed;
extern bool   config_contents_show_title;
extern bool   config_contents_show_author;
extern bool   config_contents_show_category;
extern bool   config_contents_show_date;
extern bool   config_contents_show_url;
extern bool   config_contents_show_comments;
extern char * config_contents_date_format;

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
