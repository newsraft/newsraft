#include <stdio.h>
#include <stdint.h>

uint8_t config_top_offset = 0;
uint8_t config_left_offset = 1;
int64_t config_max_items = 100;
uint8_t config_number = 1;
char config_key_download = 'd';
char config_key_download_all = 'D';

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
