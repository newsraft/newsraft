#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "newsraft.h"

static wchar_t *fmt_buf;
static size_t fmt_buf_len;
static size_t fmt_buf_lim = 200;
static struct wstring *tmp_buf;

bool
create_format_buffers(void)
{
	fmt_buf = malloc(sizeof(wchar_t) * (fmt_buf_lim + 1));
	tmp_buf = wcrtes();
	if ((fmt_buf == NULL) || (tmp_buf == NULL)) {
		fputs("Not enough memory for format buffers!\n", stderr);
		free(fmt_buf);
		free_wstring(tmp_buf);
		return false;
	}
	return true;
}

void
free_format_buffers(void)
{
	free(fmt_buf);
	free_wstring(tmp_buf);
}

static inline bool
double_format_buffer_size(void)
{
	fmt_buf_lim *= 2;
	wchar_t *dummy = realloc(fmt_buf, sizeof(wchar_t) * (fmt_buf_lim + 1));
	if (dummy == NULL) {
		return false;
	}
	fmt_buf = dummy;
	return true;
}

static inline bool
append_wchar_to_format_buffer(wchar_t wc)
{
	if ((fmt_buf_len >= fmt_buf_lim) && (double_format_buffer_size() == false)) {
		return false;
	}
	fmt_buf[fmt_buf_len++] = wc;
	return true;
}

const wchar_t *
do_format(const struct wstring *fmt, const struct format_arg *args)
{
	const wchar_t *specifier;
	int tmp_res;
	fmt_buf_len = 0;
	for (const wchar_t *iter = fmt->ptr; *iter != '\0';) {
		if (iter[0] != L'%') {
			if (append_wchar_to_format_buffer(*iter) == false) {
				return L"";
			}
			++iter;
			continue;
		} else if (iter[1] == L'%') {
			// iter[0] and iter[1] are percent signs.
			if (append_wchar_to_format_buffer(L'%') == false) {
				return L"";
			}
			iter += 2;
			continue;
		}
		// At this point iter[0] is a percent sign and iter[1] is some
		// character other than a percent sign (it may be null character).
		specifier = iter + 1;
		while ((*specifier != L'%') && (*specifier != L'\0')) {
			if (isalpha(*specifier) == 0) {
				++specifier;
				continue;
			}
			for (size_t j = 0; args[j].specifier != L'\0'; ++j) {
				if (specifier[0] != args[j].specifier) {
					continue;
				}
				if (wcpyas(tmp_buf, iter, specifier - iter) == false) {
					return L""; // OOM
				}
				if (wcatas(tmp_buf, args[j].type_specifier, wcslen(args[j].type_specifier)) == false) {
					return L""; // OOM
				}
				while (true) {
					if (wcscmp(args[j].type_specifier, L"d") == 0) {
						tmp_res = swprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, tmp_buf->ptr, args[j].value.i);
					} else if (wcscmp(args[j].type_specifier, L"s") == 0) {
						tmp_res = swprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, tmp_buf->ptr, args[j].value.s);
					} else if (wcscmp(args[j].type_specifier, L"c") == 0) {
						tmp_res = swprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, tmp_buf->ptr, args[j].value.c);
					} else { // if (wcscmp(args[j].type_specifier, L"ls") == 0) {
						tmp_res = swprintf(fmt_buf + fmt_buf_len, fmt_buf_lim + 1 - fmt_buf_len, tmp_buf->ptr, args[j].value.ls);
					}
					if (tmp_res == -1) {
						if (double_format_buffer_size() == false) {
							return L""; // OOM
						}
					} else {
						fmt_buf_len += tmp_res;
						break;
					}
				}
				break;
			}
			++specifier;
			break;
		}
		iter = specifier;
	}

	if (append_wchar_to_format_buffer(L'\0') == true) {
		return fmt_buf;
	} else {
		return L""; // OOM
	}
}
