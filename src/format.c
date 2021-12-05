#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "feedeater.h"

static char *fmt_buf = NULL;
static size_t fmt_buf_len;
static size_t fmt_buf_lim = 0;

static char *iter;
static char *next_percent;
static char *specifier;
static char specifier_tmp;

// On success returns 0.
// On failure returns non-zero.
int
reallocate_format_buffer(void)
{
	INFO("Reallocating format buffer with size of %d characters.", list_menu_width + 1);
	char *temp = realloc(fmt_buf, sizeof(char) * (list_menu_width + 1));
	if (temp != NULL) {
		fmt_buf = temp;
		fmt_buf_lim = list_menu_width;
	} else {
		FAIL("Not enough memory for reallocating format buffer!");
		free(fmt_buf);
		fmt_buf = NULL;
		return 1;
	}
	return 0;
}

// On success returns formatted string.
// On failure returns empty string.
const char *
do_format(char *fmt, struct format_arg *args, size_t args_count)
{
	if (fmt_buf == NULL) {
		FAIL("Format buffer is unset, returning empty string instead of formatted string!");
		return "";
	}
	fmt_buf_len = 0;
	iter = fmt;
	while (1) {
		if (iter[0] != '%') {
			fmt_buf[fmt_buf_len] = iter[0];
			if (iter[0] != '\0') {
				++fmt_buf_len;
				++iter;
				continue;
			} else {
				break;
			}
		}
		if (iter[1] == '%') {
			// iter[0] and iter[1] are percent signs.
			fmt_buf[fmt_buf_len++] = '%';
			iter += 2;
			continue;
		} else if (iter[1] == '\0') {
			break;
		}
		next_percent = iter + 1;
		specifier = NULL;
		while (1) {
			if (next_percent[0] == '%') {
				if (next_percent[1] == '%') {
					++next_percent;
				} else {
					break;
				}
			} else if (next_percent[0] == '\0') {
				next_percent = NULL;
				break;
			} else if ((specifier == NULL) && (isalpha(next_percent[0]) != 0)) {
				specifier = next_percent;
			}
			++next_percent;
		}
		if (next_percent != NULL) {
			next_percent[0] = '\0';
		}
		if (specifier != NULL) {
			for (size_t j = 0; j < args_count; ++j) {
				if (specifier[0] == args[j].specifier) {
					/* save original specifier */
					specifier_tmp = specifier[0];
					specifier[0] = args[j].type_specifier;
					if (specifier[0] == 'd') {
						fmt_buf_len += snprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, iter, args[j].value.i);
					} else if (specifier[0] == 's') {
						fmt_buf_len += snprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, iter, args[j].value.s);
					} else if (specifier[0] == 'c') {
						fmt_buf_len += snprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, iter, args[j].value.c);
					}
					/* restore original specifier */
					specifier[0] = specifier_tmp;
					break;
				}
			}
		}
		if (next_percent != NULL) {
			next_percent[0] = '%';
			iter = next_percent;
		}
		if (fmt_buf_len < fmt_buf_lim) {
			fmt_buf[fmt_buf_len] = '\0';
		} else {
			fmt_buf[fmt_buf_lim] = '\0';
			break;
		}
		if (next_percent == NULL) {
			break;
		}
	}
	return (const char *)fmt_buf;
}

void
free_format_buffer(void)
{
	free(fmt_buf);
}
