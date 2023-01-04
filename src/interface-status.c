#include <stdlib.h>
#include <curses.h>
#include "newsraft.h"

struct status_message {
	struct string *text;
	config_entry_id color;
};

static WINDOW *status_window = NULL;
static bool status_window_is_clean = true;
static volatile bool status_window_is_cleanable = true;
static struct status_message *messages;
static size_t messages_len = 0;
static size_t messages_lim = 0;

static inline void
write_last_status_message_to_status_window(void)
{
	mvwaddnstr(status_window, 0, 0, messages[(messages_len - 1) % messages_lim].text->ptr, list_menu_width - 9);
	wbkgd(status_window, get_color_pair(messages[(messages_len - 1) % messages_lim].color));
}

bool
status_recreate(void)
{
	if (status_window != NULL) {
		delwin(status_window);
	}
	status_window = newwin(1, list_menu_width - 9, list_menu_height, 0);
	if (status_window == NULL) {
		fputs("Failed to create status window!\n", stderr);
		return false;
	}
	INFO("Created new status window.");
	werase(status_window);
	if (status_window_is_clean == false) {
		write_last_status_message_to_status_window();
	}
	wrefresh(status_window);
	return true;
}

bool
allocate_status_messages_buffer(void)
{
	messages_lim = get_cfg_uint(CFG_STATUS_MESSAGES_COUNT_LIMIT);
	messages_lim |= 1; // Make sure it's not a zero.
	messages = calloc(messages_lim, sizeof(struct status_message));
	if (messages == NULL) {
		goto error;
	}
	for (size_t i = 0; i < messages_lim; ++i) {
		messages[i].text = crtes(100);
		if (messages[i].text == NULL) {
			goto error; // Since we calloced messages buffer, everything after that remains NULL.
		}
	}
	return true;
error:
	fputs("Not enough memory for status messages buffer!\n", stderr);
	return false;
}

void
status_clean_unprotected(void)
{
	if (status_window_is_cleanable == true) {
		status_window_is_clean = true;
		werase(status_window);
		wrefresh(status_window);
	}
}

void
status_clean(void)
{
	pthread_mutex_lock(&interface_lock);
	status_clean_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

void
prevent_status_cleaning(void)
{
	status_window_is_cleanable = false;
}

void
allow_status_cleaning(void)
{
	status_window_is_cleanable = true;
}

void
status_write(config_entry_id color, const char *format, ...)
{
	pthread_mutex_lock(&interface_lock);
	va_list args;
	va_start(args, format);
	if (string_vprintf(messages[messages_len % messages_lim].text, format, args) == false) {
		goto undo;
	}
	messages[messages_len % messages_lim].color = color;
	INFO("Printed status message: %s", messages[messages_len % messages_lim].text->ptr);
	messages_len += 1;
	status_window_is_clean = false;
	werase(status_window);
	write_last_status_message_to_status_window();
	wrefresh(status_window);
undo:
	va_end(args);
	pthread_mutex_unlock(&interface_lock);
}

struct string *
generate_string_with_status_messages_for_pager(void)
{
	struct string *str = crtes(1000);
	if (str == NULL) {
		FAIL("Not enough memory for string with status messages for pager!");
		return NULL;
	}
	size_t i = (messages_len - 1) % messages_lim;
	for (size_t j = MIN(messages_len, messages_lim); j > 0; --j) {
		catss(str, messages[i].text);
		catcs(str, '\n');
		if (i == 0) {
			i = messages_lim - 1;
		} else {
			i -= 1;
		}
	}
	return str;
}

void
status_delete(void)
{
	INFO("Freeing status messages.");
	for (size_t i = 0; i < messages_lim; ++i) {
		free_string(messages[i].text);
	}
	free(messages);
	INFO("Freeing status window.");
	delwin(status_window);
}
