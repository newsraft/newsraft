#ifndef CONFIG_H
#define CONFIG_H
#include <stdbool.h>
extern uint8_t config_top_offset;
extern uint8_t config_left_offset;
extern int64_t config_max_items;
extern uint8_t config_number;

extern bool   config_contents_show_title;
extern bool   config_contents_show_author;
extern bool   config_contents_show_date;
extern bool   config_contents_show_link;
extern char * config_contents_date_format;

extern char config_key_exit;
extern char config_key_download;
extern char config_key_download_all;
#endif // CONFIG_H
