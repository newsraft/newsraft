#include "newsraft.h"
#define TB_IMPL
#include "termbox2.h"

WINDOW *
newwin(int pos_y)
{
	WINDOW *win = newsraft_calloc(1, sizeof(*win));
	win->pos_y = pos_y;
	win->content = wcrtes(list_menu_width + 10);
	return win;
}

void
delwin(WINDOW *win)
{
	if (win) {
		free_wstring(win->content);
		newsraft_free(win);
	}
}

void
wmove(WINDOW *win, int offset_x)
{
	win->offset = offset_x;
}

void
werase(WINDOW *win)
{
	empty_wstring(win->content);
	for (int i = 0; i < tb_width(); ++i) {
		tb_set_cell(i, win->pos_y, ' ', TB_DEFAULT, TB_DEFAULT);
	}
}

uintattr_t
real_color(uintattr_t color)
{
	if (!arent_we_colorful()) {
		return TB_DEFAULT;
	}
	int mode = tb_set_output_mode(TB_OUTPUT_CURRENT);
	if (mode == TB_OUTPUT_256) {
		if (color == TB_DEFAULT) {
			return TB_DEFAULT;
		}
		if (color == TB_BLACK) {
			return TB_HI_BLACK;
		}
		return color - 1;
	}
	return color;
}

void
wbkgd(WINDOW *win, struct config_color color)
{
	int shift = 0;
	for (int i = 0; i < tb_width(); ++i) {
		if ((size_t)i < win->content->len) {
			tb_set_cell(shift, win->pos_y, win->content->ptr[i], real_color(color.fg) | color.attributes | win->attrs, real_color(color.bg));
			shift += wcwidth(win->content->ptr[i]);
		} else {
			tb_set_cell(shift, win->pos_y, ' ', real_color(color.fg) | color.attributes | win->attrs, real_color(color.bg));
			shift += 1;
		}
	}
}

void
wattrset(WINDOW *win, int attrs)
{
	win->attrs = attrs;
}

void
waddnwstr(WINDOW *win, const wchar_t *wstr, size_t lim)
{
	size_t wstr_len = wcslen(wstr);
	if (wstr_len > lim) {
		wstr_len = lim;
	}
	for (size_t i = 0; i < wstr_len; ++i) {
		int width = wcwidth(wstr[i]);
		if (width < 1) {
			WARN("Invalid wide character: %d", (int)wstr[i]);
			continue;
		}
		tb_set_cell(win->offset, win->pos_y, wstr[i], win->attrs, TB_DEFAULT);
		win->offset += width;
	}
	wcatas(win->content, wstr, wstr_len);
}

void
waddwstr(WINDOW *win, const wchar_t *wstr)
{
	waddnwstr(win, wstr, SIZE_MAX);
}

void
waddnstr(WINDOW *win, const char *str, size_t lim)
{
	struct wstring *new = convert_array_to_wstring(str, strlen(str));
	waddnwstr(win, new->ptr, lim);
	free_wstring(new);
}

void
waddstr(WINDOW *win, const char *str)
{
	waddnstr(win, str, SIZE_MAX);
}

int
get_wch(char *keyname)
{
	static const char *escape_keys[] = {
		[TB_KEY_CTRL_A]           = "^A",     // ok
		[TB_KEY_CTRL_B]           = "^B",     // ok
		[TB_KEY_CTRL_C]           = "^C",     // ok
		[TB_KEY_CTRL_D]           = "^D",     // ok
		[TB_KEY_CTRL_E]           = "^E",     // ok
		[TB_KEY_CTRL_F]           = "^F",     // ok
		[TB_KEY_CTRL_G]           = "^G",     // ok
		[TB_KEY_BACKSPACE]        = "backspace",
		[TB_KEY_TAB]              = "tab",    // ok
		[TB_KEY_CTRL_J]           = "^J",     // ok
		[TB_KEY_CTRL_K]           = "^K",     // ok
		[TB_KEY_CTRL_L]           = "^L",     // ok
		[TB_KEY_ENTER]            = "enter",  // ok
		[TB_KEY_CTRL_N]           = "^N",     // ok
		[TB_KEY_CTRL_O]           = "^O",     // ok
		[TB_KEY_CTRL_P]           = "^P",     // ok
		[TB_KEY_CTRL_Q]           = "^Q",     // ok
		[TB_KEY_CTRL_R]           = "^R",     // ok
		[TB_KEY_CTRL_S]           = "^S",     // ok
		[TB_KEY_CTRL_T]           = "^T",     // ok
		[TB_KEY_CTRL_U]           = "^U",     // ok
		[TB_KEY_CTRL_V]           = "^V",     // ok
		[TB_KEY_CTRL_W]           = "^W",     // ok
		[TB_KEY_CTRL_X]           = "^X",     // ok
		[TB_KEY_CTRL_Y]           = "^Y",     // ok
		[TB_KEY_CTRL_Z]           = "^Z",
		[TB_KEY_ESC]              = "escape", // ok
		[TB_KEY_CTRL_BACKSLASH]   = "^\\",    // ok
		[TB_KEY_CTRL_RSQ_BRACKET] = "^]",     // ok
		[TB_KEY_CTRL_6]           = "^6",     // ok
		[TB_KEY_CTRL_SLASH]       = "^/",     // ok
		[TB_KEY_SPACE]            = "space",  // ok
	};
	static const char *function_keys[] = {
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
		"KEY_INSERT",
		"KEY_DELETE",
		"KEY_HOME",
		"KEY_END",
		"KEY_PPAGE",
		"KEY_NPAGE",
		"KEY_UP",
		"KEY_DOWN",
		"KEY_LEFT",
		"KEY_RIGHT"
	};
	struct tb_event ev;
	int ret = tb_peek_event(&ev, 0);
	if (ret != TB_OK) {
		if (ret == TB_ERR_POLL && tb_last_errno() == EINTR) {
			INFO("Caught resize interrupt");
			return TB_EVENT_RESIZE;
		}
		if (ret != TB_ERR_NO_EVENT) {
			FAIL("Invalid input event, ret = %d", ret);
		}
		return TB_ERR;
	}
	if (ev.type == TB_EVENT_RESIZE) {
		INFO("Caught resize event");
		return TB_EVENT_RESIZE;
	}
	if (ev.type == TB_EVENT_KEY) {
		if (ev.key == 0) {
			int len = tb_utf8_unicode_to_char(keyname, ev.ch);
			if (len > 0 && len < 7) {
				return TB_EVENT_KEY;
			}
		} else if (ev.key < 32) {
			strcpy(keyname, escape_keys[ev.key]);
			return TB_EVENT_KEY;
		} else if (ev.key >= TB_KEY_ARROW_RIGHT) {
			strcpy(keyname, function_keys[0xFFFF - ev.key]);
			return TB_EVENT_KEY;
		} else if (ev.key == TB_KEY_CTRL_8 || ev.key == TB_KEY_BACKSPACE2) {
			strcpy(keyname, "backspace");
			return TB_EVENT_KEY;
		}
	}
	WARN("Unknown input event, key = %u, char = %u", ev.key, ev.ch);
	return TB_ERR;
}
