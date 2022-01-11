#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "feedeater.h"

static wchar_t *fmt_buf;

// On success returns true.
// On memory shortage returns false.
bool
adjust_list_menu_format_buffer(void)
{
	wchar_t *temp = realloc(fmt_buf, sizeof(wchar_t) * (list_menu_width + 1));
	if (temp == NULL) {
		FAIL("Adjustment of list menu format buffer failed!");
		return false;
	}
	fmt_buf = temp;
	return true;
}

void
free_list_menu_format_buffer(void)
{
	free(fmt_buf);
}

// On success returns formatted string.
// On failure returns empty string.
const wchar_t *
do_format(const wchar_t *fmt, const struct format_arg *args, size_t args_count)
{
	const wchar_t *iter = fmt;
	const wchar_t *next_percent;
	const wchar_t *specifier;
	wchar_t word[3333];
	size_t fmt_buf_len = 0;
	while ((iter[0] != L'\0') && (fmt_buf_len < list_menu_width)) {
		if (iter[0] != L'%') {
			fmt_buf[fmt_buf_len++] = iter[0];
			++iter;
			continue;
		} else if (iter[1] == L'%') {
			// iter[0] and iter[1] are percent signs.
			fmt_buf[fmt_buf_len++] = L'%';
			iter += 2;
			continue;
		} else if (iter[1] == L'\0') {
			break;
		}
		next_percent = iter + 1;
		specifier = NULL;
		while (1) {
			if (next_percent[0] == L'%') {
				if (next_percent[1] == L'%') {
					fmt_buf[fmt_buf_len++] = L'%';
					next_percent += 2;
					continue;
				} else {
					break;
				}
			} else if (next_percent[0] == L'\0') {
				break;
			} else if ((specifier == NULL) && (isalpha(next_percent[0]) != 0)) {
				specifier = next_percent;
			}
			++next_percent;
		}
		if (fmt_buf_len >= list_menu_width) {
			break;
		}
		if (specifier != NULL) {
			for (size_t j = 0; j < args_count; ++j) {
				if (specifier[0] != args[j].specifier) {
					continue;
				}
				wcsncpy(word, iter, specifier - iter);
				wcscpy(word + (specifier - iter), args[j].type_specifier);
				wcsncat(word, specifier + 1, next_percent - (specifier + 1));
				if (wcscmp(args[j].type_specifier, L"d") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, word, args[j].value.i);
				} else if (wcscmp(args[j].type_specifier, L"s") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, word, args[j].value.s);
				} else if (wcscmp(args[j].type_specifier, L"c") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, word, args[j].value.c);
				} else if (wcscmp(args[j].type_specifier, L"ls") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, word, args[j].value.ls);
				}
				break;
			}
		}
		iter = next_percent;
	}

	if (fmt_buf_len > list_menu_width) {
		fmt_buf[list_menu_width] = L'\0';
	} else {
		fmt_buf[fmt_buf_len] = L'\0';
	}

	return (const wchar_t *)fmt_buf;
}
