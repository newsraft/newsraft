#include <ctype.h>
#include "newsraft.h"

static struct wstring *fmt_buf;
static struct wstring *tmp_buf;

bool
create_format_buffers(void)
{
	fmt_buf = wcrtes(1000);
	tmp_buf = wcrtes(100);
	if ((fmt_buf == NULL) || (tmp_buf == NULL)) {
		fputs("Not enough memory for format buffers!\n", stderr);
		free_wstring(fmt_buf);
		free_wstring(tmp_buf);
		return false;
	}
	return true;
}

void
free_format_buffers(void)
{
	free_wstring(fmt_buf);
	free_wstring(tmp_buf);
}

const struct wstring *
do_format(const wchar_t *fmt, const struct format_arg *args)
{
	const wchar_t *specifier;
	int tmp_res;
	empty_wstring(fmt_buf);
	for (const wchar_t *iter = fmt; *iter != '\0';) {
		if (iter[0] != L'%') {
			if (wcatcs(fmt_buf, *iter) == false) {
				return tmp_buf; // OOM
			}
			iter += 1;
			continue;
		} else if (iter[1] == L'%') {
			// iter[0] and iter[1] are percent signs.
			if (wcatcs(fmt_buf, L'%') == false) {
				return tmp_buf; // OOM
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
				if (wcpyas(tmp_buf, iter, specifier - iter) == false) {
					return tmp_buf; // OOM
				}
				if (wcatcs(tmp_buf, args[j].type_specifier) == false) {
					return tmp_buf; // OOM
				}
				do {
					if (args[j].type_specifier == L's') {
						tmp_res = swprintf(fmt_buf->ptr + fmt_buf->len, fmt_buf->lim + 1 - fmt_buf->len, tmp_buf->ptr, args[j].value.s);
					} else {
						tmp_res = swprintf(fmt_buf->ptr + fmt_buf->len, fmt_buf->lim + 1 - fmt_buf->len, tmp_buf->ptr, args[j].value.i);
					}
					if (tmp_res != -1) {
						fmt_buf->len += tmp_res;
						fmt_buf->ptr[fmt_buf->len] = L'\0';
					} else if (increase_wstring_size(fmt_buf, 100) == false) {
						return tmp_buf; // OOM
					}
				} while (tmp_res == -1);
				break;
			}
			specifier += 1;
			break;
		}
		iter = specifier;
	}
	return fmt_buf;
}
