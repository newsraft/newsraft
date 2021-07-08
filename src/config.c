#include <stdio.h>
#include <stdbool.h>
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

// if cur_char is equal to one of the elements from list then
// set position indicator of file to next character that mismatched all elements in list
// or to the end of file
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

// if cur_char is not equal to all elements from list then
// set position indicator of file to next character that matched one of the elements in list
// or to the end of file
void
find_chars(FILE *file, char *cur_char, char *list)
{
	bool match = false;
	uint8_t i;
	while (1) {
		i = 0;
		while (list[i] != '\0') {
			if (*cur_char == list[i]) {
				match = true;
				break;
			}
			++i;
		}
		if (match == true) {
			break;
		} else {
			*cur_char = fgetc(file);
			if (*cur_char == EOF) break;
		}
	}
}
