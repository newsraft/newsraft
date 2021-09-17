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
	char *iter = config_menu_set_entry_format;
	char *percent_ptr = strchr(iter, '%');

	if (percent_ptr == NULL) {
		/* format string has no specifiers (wonder why),
		 * so just print its contents to set's window */
		wprintw(set->window, "%s", iter);
		return;
	}

	/* write everything before the first specifier to set's window */
	*(percent_ptr) = '\0';
	wprintw(set->window, "%s", iter);
	*(percent_ptr) = '%';

	iter = percent_ptr + 1;

	size_t i;
	char old_char;
	while (*iter != '\0') {
		percent_ptr = strchr(iter, '%');
		if (percent_ptr != NULL) {
			if (percent_ptr == iter) {
				/* met %% */
				percent_ptr = strchr(iter + 1, '%');
				if (percent_ptr != NULL) {
					*(percent_ptr) = '\0';
				}
			} else {
				*(percent_ptr) = '\0';
			}
		}

		// iter[0] is the first character after %
		// here we find specifier character position
		i = 0;
		while (isdigit(iter[i]) || iter[i] == '-') {
			++i;
		}

		/* save original specifier */
		old_char = iter[i];

		if (iter[i] == 'n') {
			if (set->unread_count != 0) {
				iter[i] = 'd';
				wprintw(set->window, iter - 1, set->unread_count);
			} else {
				iter[i] = 's';
				wprintw(set->window, iter - 1, "");
			}
		} else if (iter[i] == 'N') {
			iter[i] = 'd';
			wprintw(set->window, iter - 1, set->unread_count);
		} else if (iter[i] == 't') {
			iter[i] = 's';
			wprintw(set->window, iter - 1, set_image(set));
		} else if (iter[i] == 'i') {
			if ((set->link == NULL) && (set->tags == NULL)) {
				iter[i] = 's';
				wprintw(set->window, iter - 1, "");
			} else {
				iter[i] = 'd';
				wprintw(set->window, iter - 1, index + 1);
			}
		} else if (iter[i] == 'I') {
			wprintw(set->window, iter - 1, index + 1);
		} else {
			wprintw(set->window, iter - 1);
		}

		/* restore original specifier */
		iter[i] = old_char;

		if (percent_ptr != NULL) {
			*(percent_ptr) = '%';
			iter = percent_ptr + 1;
		} else {
			break;
		}
	}
}
