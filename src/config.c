#include <stdio.h>
#include <stdint.h>

uint8_t left_offset = 1;

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
