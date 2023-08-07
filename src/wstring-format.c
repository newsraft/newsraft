#include <ctype.h>
#include <string.h>
#include "newsraft.h"

#define FORMAT_TMP_BUF_SIZE 200

void
do_format(struct wstring *dest, const wchar_t *fmt, const struct format_arg *args)
{
	wchar_t tmp_buf[FORMAT_TMP_BUF_SIZE + 2];
	const wchar_t *specifier;
	int tmp_res;
	empty_wstring(dest);
	for (const wchar_t *iter = fmt; *iter != '\0';) {
		if (iter[0] != L'%') {
			if (wcatcs(dest, *iter) == false) {
				return;
			}
			iter += 1;
			continue;
		} else if (iter[1] == L'%') {
			// iter[0] and iter[1] are percent signs.
			if (wcatcs(dest, L'%') == false) {
				return;
			}
			iter += 2;
			continue;
		}
		// At this point iter[0] is a percent sign and iter[1] is some
		// character other than a percent sign (it may be null character).
		for (specifier = iter + 1; (*specifier != L'%') && (*specifier != L'\0'); ++specifier) {
			if (isalpha(*specifier) == 0) {
				continue;
			}
			for (size_t j = 0; args[j].specifier != L'\0'; ++j) {
				if (specifier[0] != args[j].specifier) {
					continue;
				}
				if (specifier - iter > FORMAT_TMP_BUF_SIZE) {
					return;
				}
				memcpy(tmp_buf, iter, sizeof(wchar_t) * (specifier - iter));
				tmp_buf[specifier - iter] = args[j].type_specifier;
				tmp_buf[specifier - iter + 1] = L'\0';
				do {
					if (args[j].type_specifier == L's') {
						tmp_res = swprintf(dest->ptr + dest->len, dest->lim + 1 - dest->len, tmp_buf, args[j].value.s);
					} else {
						tmp_res = swprintf(dest->ptr + dest->len, dest->lim + 1 - dest->len, tmp_buf, args[j].value.i);
					}
					if (tmp_res != -1) {
						dest->len += tmp_res;
						dest->ptr[dest->len] = L'\0';
					} else if (increase_wstring_size(dest, 100) == false) {
						return;
					}
				} while (tmp_res == -1);
				break;
			}
			specifier += 1;
			break;
		}
		iter = specifier;
	}
}
