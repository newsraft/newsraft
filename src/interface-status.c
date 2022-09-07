#include <stdlib.h>
#include <pthread.h>
#include "newsraft.h"

enum {
	STATUS_GOOD,
	STATUS_INFO,
	STATUS_FAIL,
};

struct status_message {
	struct string *text;
	int8_t condition; // What condition my condition was in?
};

static WINDOW *status_window;
static bool status_window_is_clean;
static struct status_message *messages;
static size_t messages_count;
static size_t messages_limit;
static pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;

static inline WINDOW *
create_status_window(void)
{
	WINDOW *win = newwin(1, list_menu_width - 9, list_menu_height, 0);
	if (win != NULL) {
		INFO("Created new status window.");
		if (keypad(win, TRUE) == ERR) {
			WARN("Can't enable keypad and function keys for status window!");
		}
	} else {
		FAIL("Failed to create new status window!");
	}
	return win;
}

bool
status_create(void)
{
	status_window = create_status_window();
	if (status_window == NULL) {
		fputs("Failed to create status field window!\n", stderr);
		return false;
	}
	status_window_is_clean = true;
	messages = NULL;
	messages_count = 0;
	messages_limit = get_cfg_uint(CFG_STATUS_MESSAGES_COUNT_LIMIT);
	return true;
}

static inline void
write_last_status_message_to_status_window(void)
{
	if ((messages != NULL) && (messages_count != 0)) {
		struct status_message *m;
		if (messages_limit == 0) {
			m = messages + messages_count - 1;
		} else {
			m = messages + ((messages_count - 1) % messages_limit);
		}
		mvwaddnstr(status_window, 0, 0, m->text->ptr, list_menu_width - 9);
		if (m->condition == STATUS_GOOD) {
			wbkgd(status_window, get_color_pair(CFG_COLOR_STATUS_GOOD_FG));
		} else if (m->condition == STATUS_INFO) {
			wbkgd(status_window, get_color_pair(CFG_COLOR_STATUS_INFO_FG));
		} else {
			wbkgd(status_window, get_color_pair(CFG_COLOR_STATUS_FAIL_FG));
		}
	}
}

void
status_update(void)
{
	werase(status_window);
	if (status_window_is_clean == false) {
		write_last_status_message_to_status_window();
	}
	wrefresh(status_window);
}

static inline bool
append_new_message_to_status_messages(int8_t new_condition, struct string *new_message)
{
	messages_count += 1;
	if ((messages_limit == 0) || (messages_count <= messages_limit)) {
		struct status_message *temp = realloc(messages, sizeof(struct status_message) * messages_count);
		if (temp == NULL) {
			return false;
		}
		messages = temp;
		messages[messages_count - 1].text = new_message;
		messages[messages_count - 1].condition = new_condition;
	} else {
		free_string(messages[(messages_count - 1) % messages_limit].text);
		messages[(messages_count - 1) % messages_limit].text = new_message;
		messages[(messages_count - 1) % messages_limit].condition = new_condition;
	}
	return true;
}

void
status_write(int8_t condition, const char *format, ...)
{
	pthread_mutex_lock(&write_lock);
	va_list args;
	va_start(args, format);
	struct string *new_message = crtes();
	if (new_message == NULL) {
		goto undo;
	}
	if (string_vprintf(new_message, format, args) == false) {
		free_string(new_message);
		goto undo;
	}
	if (append_new_message_to_status_messages(condition, new_message) == false) {
		free_string(new_message);
		goto undo;
	}
	INFO("Last status message: %s", new_message->ptr);
	status_window_is_clean = false;
	status_update();
undo:
	va_end(args);
	pthread_mutex_unlock(&write_lock);
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
	if ((messages_limit == 0) || (messages_count <= messages_limit)) {
		for (size_t i = messages_count; i != 0; --i) {
			catss(str, messages[i - 1].text);
			catcs(str, '\n');
		}
	} else {
		size_t i = (messages_count - 1) % messages_limit;
		for (size_t count = messages_limit; count != 0; --count) {
			catss(str, messages[i].text);
			catcs(str, '\n');
			if (i == 0) {
				i = messages_limit - 1;
			} else {
				i = i - 1;
			}
		}
	}
	return str;
}

static inline void
free_messages(void)
{
	if ((messages_limit == 0) || (messages_count <= messages_limit)) {
		for (size_t i = 0; i < messages_count; ++i) {
			free_string(messages[i].text);
		}
	} else {
		for (size_t i = 0; i < messages_limit; ++i) {
			free_string(messages[i].text);
		}
	}
	free(messages);
}

void
status_delete(void)
{
	INFO("Freeing status messages.");
	free_messages();
	INFO("Freeing status window.");
	delwin(status_window);
}
