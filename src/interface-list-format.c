#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "feedeater.h"

static wchar_t *fmt_buf;
static wchar_t tmp[FORMAT_STRING_LENGTH_LIMIT + 1];

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
do_format(int format_setting, const struct format_arg *args, size_t args_count)
{
	const struct wstring *fmt = get_cfg_wstring(format_setting);
	const wchar_t *iter = fmt->ptr;
	const wchar_t *next_percent;
	const wchar_t *specifier;
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
		// At this point iter[0] is percent sign and iter[1] is some character
		// other than a percent sign and a null terminator.
		next_percent = iter + 1;
		specifier = NULL;
		while ((specifier == NULL) && (next_percent[0] != L'%') && (next_percent[0] != L'\0')) {
			if (isalpha(next_percent[0]) != 0) {
				specifier = next_percent;
			}
			++next_percent;
		}
		if (specifier != NULL) {
			for (size_t j = 0; j < args_count; ++j) {
				if (specifier[0] != args[j].specifier) {
					continue;
				}
				wcsncpy(tmp, iter, specifier - iter);
				wcscpy(tmp + (specifier - iter), args[j].type_specifier);
				wcsncat(tmp, specifier + 1, next_percent - (specifier + 1));
				if (wcscmp(args[j].type_specifier, L"d") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, tmp, args[j].value.i);
				} else if (wcscmp(args[j].type_specifier, L"s") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, tmp, args[j].value.s);
				} else if (wcscmp(args[j].type_specifier, L"c") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, tmp, args[j].value.c);
				} else if (wcscmp(args[j].type_specifier, L"ls") == 0) {
					fmt_buf_len += swprintf(fmt_buf + fmt_buf_len, list_menu_width + 1 - fmt_buf_len, tmp, args[j].value.ls);
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
