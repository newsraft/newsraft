#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

uint8_t config_top_offset = 0;
uint8_t config_left_offset = 1;
int64_t config_max_items = 100;
uint8_t config_number = 1;

bool config_contents_show_feed = true;
bool config_contents_show_title = true;
bool config_contents_show_author = true;
bool config_contents_show_date = true;
bool config_contents_show_link = true;
char *config_contents_date_format = "%a, %d %b %Y %H:%M:%S %z";

char config_key_mark_read = 'r';
char config_key_mark_read_all = 'R';
char config_key_mark_unread = 'u';
char config_key_mark_unread_all = 'U';
char config_key_download = 'd';
char config_key_download_all = 'D';
char config_key_exit = 'q';

// if cur_char is equal to one of the elements from list, then
// set position indicator of file to next character that mismatched all elements in list
void
skip_chars(FILE *file, char *cur_char, char *list)
{
	uint8_t i = 0;
	while (list[i] != '\0') {
		if (*cur_char == list[i]) {
			*cur_char = fgetc(file);
			i = 0;
		} else {
			++i;
		}
	}
}
