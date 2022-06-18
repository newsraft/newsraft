#include <stdlib.h>
#include "newsraft.h"

static WINDOW *status_window;
static bool status_window_is_clean;
static struct string **status_messages;
static size_t status_messages_count;
static size_t status_messages_limit;

static inline WINDOW *
create_status_window(void)
{
	WINDOW *win;
	if (list_menu_width > 9) {
		win = newwin(1, list_menu_width - 9, list_menu_height, 0);
	} else {
		win = newwin(1, list_menu_width, list_menu_height, 0);
	}
	if (win != NULL) {
		INFO("Created new status window.");
		if (keypad(win, TRUE) == ERR) {
			WARN("Can't enable keypad and function keys for status window!");
		}
		wbkgd(win, get_color_pair(CFG_COLOR_STATUS_FG));
	} else {
		FAIL("Failed to create new status window!");
	}
	return win;
}

static inline void
write_to_status_window(const char *src)
{
	if (list_menu_width > 9) {
		mvwaddnstr(status_window, 0, 0, src, list_menu_width - 9);
	} else {
		mvwaddnstr(status_window, 0, 0, src, list_menu_width);
	}
}

bool
status_create(void)
{
	status_window = create_status_window();
	if (status_window == NULL) {
		fprintf(stderr, "Failed to create status line window!\n");
		return false;
	}
	status_window_is_clean = true;
	status_messages = NULL;
	status_messages_count = 0;
	status_messages_limit = get_cfg_uint(CFG_STATUS_MESSAGES_LIMIT);
	return true;
}

static inline const struct string *
get_last_status_message(void)
{
	if (status_messages_count == 0) {
		return NULL;
	}
	if (status_messages_limit == 0) {
		return status_messages[status_messages_count - 1];
	}
	return status_messages[(status_messages_count - 1) % status_messages_limit];
}

void
status_update(void)
{
	werase(status_window);
	if (status_window_is_clean == false) {
		const struct string *last_status_message = get_last_status_message();
		if (last_status_message != NULL) {
			write_to_status_window(last_status_message->ptr);
		}
	}
	wrefresh(status_window);
}

static inline bool
append_new_message_to_status_messages(struct string *new_message)
{
	status_messages_count += 1;
	if (status_messages_limit == 0) {
		struct string **temp = realloc(status_messages, sizeof(struct string *) * status_messages_count);
		if (temp == NULL) {
			return false;
		}
		status_messages = temp;
		status_messages[status_messages_count - 1] = new_message;
	} else {
		if (status_messages_count <= status_messages_limit) {
			struct string **temp = realloc(status_messages, sizeof(struct string *) * status_messages_count);
			if (temp == NULL) {
				return false;
			}
			status_messages = temp;
			status_messages[(status_messages_count - 1) % status_messages_limit] = new_message;
		} else {
			free_string(status_messages[(status_messages_count - 1) % status_messages_limit]);
			status_messages[(status_messages_count - 1) % status_messages_limit] = new_message;
		}
	}
	return true;
}

void
status_write(const char *format, ...)
{
	struct string *new_message = crtes();
	if (new_message == NULL) {
		return;
	}
	va_list args;
	va_start(args, format);
	if (string_vprintf(new_message, format, args) == false) {
		va_end(args);
		free_string(new_message);
		return;
	}
	va_end(args);
	if (append_new_message_to_status_messages(new_message) == false) {
		free_string(new_message);
		return;
	}
	status_window_is_clean = false;
	status_update();
	INFO("Last status message: %s", new_message->ptr);
}

void
status_clean(void)
{
	status_window_is_clean = true;
	werase(status_window);
	wrefresh(status_window);
}

bool
status_resize(void)
{
	if (status_window != NULL) {
		delwin(status_window);
	}
	status_window = create_status_window();
	if (status_window != NULL) {
		status_update();
	} else {
		return false;
	}
	return true;
}

int
read_key_from_status(void)
{
	// We can't read keys from stdscr via getch() function because calling it
	// will bring stdscr on top of other windows and overlap them.
	int c = wgetch(status_window);
	INFO("Received \"%c\" character with %d key code.", c, c);
	return c;
}

struct string *
generate_string_with_status_messages_for_pager(void)
{
	struct string *str = crtes();
	if (str == NULL) {
		FAIL("Not enough memory for string with status messages for pager!");
		return NULL;
	}
	if ((status_messages_limit == 0) || (status_messages_count <= status_messages_limit)) {
		size_t i = status_messages_count;
		while (i > 0) {
			i = i - 1;
			catss(str, status_messages[i]);
			catcs(str, '\n');
		}
	} else {
		size_t i = (status_messages_count - 1) % status_messages_limit;
		for (size_t messages_count = status_messages_limit; messages_count != 0; messages_count--) {
			catss(str, status_messages[i]);
			catcs(str, '\n');
			if (i == 0) {
				i = status_messages_limit - 1;
			} else {
				i = i - 1;
			}
		}
	}
	return str;
}

static inline void
free_status_messages(void)
{
	if ((status_messages_limit == 0) || (status_messages_count < status_messages_limit)) {
		for (size_t i = 0; i < status_messages_count; ++i) {
			free_string(status_messages[i]);
		}
	} else {
		for (size_t i = 0; i < status_messages_limit; ++i) {
			free_string(status_messages[i]);
		}
	}
	free(status_messages);
}

void
status_delete(void)
{
	INFO("Freeing status window and status messages.");
	delwin(status_window);
	free_status_messages();
}
