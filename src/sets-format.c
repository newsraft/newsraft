#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "feedeater.h"

// return most sensible string for set line
static char *
set_image(struct set_line *set)
{
	if (set->name != NULL) {
		return set->name->ptr;
	} else {
		if (set->link != NULL) {
			return set->link->ptr;
		} else {
			if (set->tags != NULL) {
				return set->tags->ptr;
			} else {
				return "";
			}
		}
	}
}

void
print_set_format(size_t index, struct set_line *set)
{
	size_t fmt_len = strlen(config_menu_set_entry_format);
	char *fmt = malloc(sizeof(char) * (fmt_len + 1));;
	if (fmt == NULL) {
		return;
	}
	strcpy(fmt, config_menu_set_entry_format);

	char *iter = fmt;
	char *percent_ptr = strchr(iter, '%');

	if (percent_ptr == NULL) { /* no conversions */
		wprintw(set->window, "%s", iter);
		free(fmt);
		return;
	}

	/* write everything before the first conversion */
	*(percent_ptr) = '\0';
	wprintw(set->window, "%s", iter);
	*(percent_ptr) = '%';
	iter = percent_ptr + 1;

	size_t i;
	char word[1000];
	bool done = false;
	while (done == false && *iter != '\0') {
		percent_ptr = strchr(iter, '%');
		if (percent_ptr == NULL) {
			strcpy(word, iter - 1);
			done = true;
		} else {
			*(percent_ptr) = '\0';
			strcpy(word, iter - 1);
			*(percent_ptr) = '%';

			if (iter == percent_ptr) { // met %%
				wprintw(set->window, "%s", "%");
			}

			iter = percent_ptr + 1;
		}

		// first char of word is %, get to specifier
		i = 1;
		while (isdigit(word[i]) || word[i] == '-') {
			++i;
		}

		if (word[i] == '\0') {
			continue;
		} else if (word[i] == 'n') {
			if (set->unread_count != 0) {
				word[i] = 'd';
				wprintw(set->window, word, set->unread_count);
			} else {
				word[i] = 's';
				wprintw(set->window, word, "");
			}
		} else if (word[i] == 'N') {
			word[i] = 'd';
			wprintw(set->window, word, set->unread_count);
		} else if (word[i] == 't') {
			word[i] = 's';
			wprintw(set->window, word, set_image(set));
		} else if (word[i] == 'i') {
			wprintw(set->window, word, index + 1);
		} else {
			wprintw(set->window, "%s", word + i);
		}
	}
	free(fmt);
}
