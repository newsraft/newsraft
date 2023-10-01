#include <ctype.h>
#include <string.h>
#include "newsraft.h"

#define FORMAT_TMP_BUF_SIZE 200

void
do_format(struct wstring *dest, const wchar_t *fmt, const struct format_arg *args)
{
	wchar_t tmp_buf[FORMAT_TMP_BUF_SIZE + 2];
	empty_wstring(dest);
	for (const wchar_t *iter = fmt; *iter != '\0';) {
		if (iter[0] != L'%') {
			if (wcatcs(dest, *iter) == false) return;
			iter += 1;
			continue;
		} else if (iter[1] == L'%') { // iter[0] and iter[1] are percent signs.
			if (wcatcs(dest, L'%') == false) return;
			iter += 2;
			continue;
		}
		const wchar_t *percent = iter;
		bool inserted = false;
		for (iter = iter + 1; inserted != true && *iter != L'%' && *iter != L'\0'; ++iter) {
			for (size_t j = 0; args[j].specifier != L'\0'; ++j) {
				if (*iter != args[j].specifier) {
					continue;
				}
				if (iter - percent > FORMAT_TMP_BUF_SIZE) {
					return;
				}
				memcpy(tmp_buf, percent, sizeof(wchar_t) * (iter - percent));
				tmp_buf[iter - percent] = args[j].type_specifier;
				tmp_buf[iter - percent + 1] = L'\0';
				if (args[j].type_specifier == L's') {
					// Can't rely on swprintf here because it doesn't take into account
					// double-width characters (for example emojis or chinese symbols).
					// When displaying these characters, they take two widths of regular
					// character and printf family of functions process strings based on
					// their character length rather than character width.
					struct wstring *ws = convert_array_to_wstring(args[j].value.s, strlen(args[j].value.s));
					long need_len = wcstol(tmp_buf + 1, NULL, 10);
					if (need_len != 0) {
						bool left_adjusted;
						if (need_len < 0) {
							left_adjusted = true;
							need_len = -need_len;
						} else {
							left_adjusted = false;
						}
						while (wcswidth(ws->ptr, ws->len) > need_len) {
							ws->len -= 1;
							ws->ptr[ws->len] = L'\0';
						}
						long whitespace_len = need_len - wcswidth(ws->ptr, ws->len);
						if (whitespace_len > 0 && whitespace_len <= need_len) {
							if (left_adjusted == true) {
								for (long i = 0; i < whitespace_len; ++i) {
									wcatcs(ws, L' ');
								}
							} else {
								struct wstring *new_ws = wcrtes(whitespace_len + ws->lim);
								for (long i = 0; i < whitespace_len; ++i) {
									wcatcs(new_ws, L' ');
								}
								wcatss(new_ws, ws);
								free_wstring(ws);
								ws = new_ws;
							}
						}
					}
					if (wcatss(dest, ws) == false) {
						free_wstring(ws);
						return;
					}
					free_wstring(ws);
				} else { // Format integer. Any integer is shorter than 50 characters.
					if (make_sure_there_is_enough_space_in_wstring(dest, 50) == false) return;
					int chunk_size = swprintf(dest->ptr + dest->len, dest->lim + 1 - dest->len, tmp_buf, args[j].value.i);
					if (chunk_size < 0) return;
					dest->len += chunk_size;
					dest->ptr[dest->len] = L'\0';
				}
				inserted = true;
				break;
			}
		}
	}
}
