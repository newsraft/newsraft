#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "feedeater.h"

void
super_format_2000(WINDOW *window, char *fmt, struct format_arg *args, size_t args_count)
{
	char *iter = fmt;
	char *percent_ptr = strchr(iter, '%');

	if (percent_ptr == NULL) {
		/* format string has no specifiers (wonder why),
		 * so just print its contents to window */
		wprintw(window, "%s", iter);
		return;
	}

	/* write everything before the first specifier to set's window */
	*(percent_ptr) = '\0';
	wprintw(window, "%s", iter);
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
		while ((isalpha(iter[i]) == 0) && (iter[i] != '\0')) {
			++i;
		}

		for (size_t j = 0; j < args_count; ++j) {
			if (iter[i] == args[j].specifier) {
				/* save original specifier */
				old_char = iter[i];
				iter[i] = args[j].type_specifier;
				if (args[j].type_specifier == 'd') {
					wprintw(window, iter - 1, args[j].value.i);
				} else if (args[j].type_specifier == 's') {
					wprintw(window, iter - 1, args[j].value.s);
				} else if (args[j].type_specifier == 'c') {
					wprintw(window, iter - 1, args[j].value.c);
				}
				/* restore original specifier */
				iter[i] = old_char;
				break;
			}
		}

		if (percent_ptr == NULL) {
			break;
		}

		*(percent_ptr) = '%';
		iter = percent_ptr + 1;
	}
}
