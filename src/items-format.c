#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "feedeater.h"

void
print_item_format(size_t index, const struct item_line *item)
{
	char *iter = config_menu_item_entry_format;
	char *percent_ptr = strchr(iter, '%');

	if (percent_ptr == NULL) {
		/* format string has no specifiers (wonder why),
		 * so just print its contents to item's window */
		wprintw(item->window, "%s", iter);
		return;
	}

	/* write everything before the first specifier to item's window */
	*(percent_ptr) = '\0';
	wprintw(item->window, "%s", iter);
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

		/* find specifier character position */
		i = 0;
		while (isalpha(iter[i]) == 0 && iter[i] != '\0') {
			++i;
		}

		/* save original specifier */
		old_char = iter[i];

		if (iter[i] == 'n') {
			iter[i] = 's';
			wprintw(item->window, iter - 1, item->is_unread == 0 ? " " : "N");
		} else if (iter[i] == 't') {
			iter[i] = 's';
			wprintw(item->window, iter - 1, item->title->ptr);
		} else if (iter[i] == 'i' || iter[i] == 'I') {
			iter[i] = 'd';
			wprintw(item->window, iter - 1, index + 1);
		} else {
			wprintw(item->window, iter - 1);
		}

		/* restore original specifier */
		iter[i] = old_char;

		if (percent_ptr == NULL) {
			break;
		}

		*(percent_ptr) = '%';
		iter = percent_ptr + 1;
	}
}
