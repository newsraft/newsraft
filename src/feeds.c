#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static struct feed_line **feeds = NULL;
static size_t feeds_count = 0;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"d", {.i = 0}},
	{L't', L"s", {.s = NULL}},
};

// On success returns 0.
// On failure returns non-zero.
static inline bool
parse_feeds_file(void)
{
	const char *path = get_feeds_path();
	if (path == NULL) {
		// Error message is written by get_feeds_path().
		return false;
	}
	struct string *word = crtes();
	if (word == NULL) {
		return false;
	}
	struct string *section_name = crtas(cfg.global_section_name, strlen(cfg.global_section_name));
	if (section_name == NULL) {
		free_string(word);
		return false;
	}
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "Could not open feeds file!\n");
		free_string(word);
		free_string(section_name);
		return false;
	}
	struct feed_line *feed;
	char c;

	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (true) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (ISWHITESPACE(c));

		if (c == '#') {
			// Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == '\n') {
				continue;
			} else {
				break;
			}
		} else if (c == '[') {
			empty_string(section_name);
			while (true) {
				c = fgetc(f);
				if (c == ']' || c == '\n' || c == EOF) { break; }
				if (catcs(section_name, c) == false) { goto error; }
			}
			if (c == ']') {
				do { c = fgetc(f); } while ((c != '\n') && (c != EOF));
			}
			if (c == EOF) {
				break;
			}
			continue;
		} else if (c == EOF) {
			break;
		}

		feed = malloc(sizeof(struct feed_line));
		if (feed == NULL) { goto error; }
		feed->name = NULL;
		feed->link = NULL;
		feed->unread_count = 0;

		empty_string(word);
		while (true) {
			if (catcs(word, c) == false) { goto error; }
			c = fgetc(f);
			if (ISWHITESPACE(c) || c == EOF) { break; }
		}
		if (check_url_for_validity(word) == false) {
			fprintf(stderr, "Stumbled across an invalid URL: \"%s\"!\n", word->ptr);
			goto error;
		}
		remove_trailing_slash_from_string(word);
		feed->link = crtss(word);
		if (feed->link == NULL) { goto error; }
		feed->unread_count = get_unread_items_count(feed->link);
		while (ISWHITESPACEEXCEPTNEWLINE(c)) { c = fgetc(f); }
		// process name
		if (c == '"') {
			empty_string(word);
			while (true) {
				c = fgetc(f);
				if (c == '"' || c == '\n' || c == EOF) { break; }
				if (catcs(word, c) == false) { goto error; }
			}
			feed->name = crtss(word);
			if (feed->name == NULL) { goto error; }
		}
		if (add_feed_to_section(feed, section_name) == false) {
			fprintf(stderr, "Failed to add feed \"%s\" to section \"%s\"!\n", feed->link->ptr, section_name->ptr);
			goto error;
		}

		// Skip everything to the next newline character.
		if (c != '\n') {
			if (c == EOF) {
				break;
			}
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == EOF) {
				break;
			}
		}

	}

	free_string(word);
	free_string(section_name);
	fclose(f);
	return true;
error:
	if (feed != NULL) {
		free_string(feed->name);
		free_string(feed->link);
		free(feed);
	}
	free_string(word);
	free_string(section_name);
	fclose(f);
	free_sections();
	return false;
}

bool
load_feeds(void)
{
	if (create_global_section() == false) {
		fprintf(stderr, "Not enough memory for global section structure!\n");
		return false;
	}

	if (parse_feeds_file() == false) {
		fprintf(stderr, "Failed to load feeds from file!\n");
		return false;
	}

	return true;
}

// Returns most sensible string for feed entry.
static inline char *
feed_image(const struct feed_line *feed)
{
	if (feed->name != NULL) {
		return feed->name->ptr;
	} else {
		if (feed->link != NULL) {
			return feed->link->ptr;
		} else {
			return "";
		}
	}
}

static void
feed_expose(size_t index)
{
	werase(feeds[index]->window);
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = feeds[index]->unread_count;
	fmt_args[2].value.s = feed_image(feeds[index]);
	mvwaddnwstr(feeds[index]->window, 0, 0, do_format(cfg.menu_feed_entry_format, fmt_args, COUNTOF(fmt_args)), list_menu_width);
	mvwchgat(feeds[index]->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(feeds[index]->window);
}

static void
show_feeds(void)
{
	for (size_t i = view_min, j = 0; i < feeds_count && i <= view_max; ++i, ++j) {
		feeds[i]->window = get_list_entry_by_index(j);
		feed_expose(i);
	}
}

static void
view_select(size_t i)
{
	size_t new_sel = i;

	if (new_sel >= feeds_count) {
		if (feeds_count == 0) {
			return;
		}
		new_sel = feeds_count - 1;
	}

	if (new_sel == view_sel) {
		return;
	}

	if (new_sel > view_max) {
		view_min = new_sel - (list_menu_height - 1);
		view_max = new_sel;
		view_sel = new_sel;
		show_feeds();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + (list_menu_height - 1);
		view_sel = new_sel;
		show_feeds();
	} else {
		mvwchgat(feeds[view_sel]->window, 0, 0, -1, A_NORMAL, 0, NULL);
		wrefresh(feeds[view_sel]->window);
		view_sel = new_sel;
		mvwchgat(feeds[view_sel]->window, 0, 0, -1, A_REVERSE, 0, NULL);
		wrefresh(feeds[view_sel]->window);
	}
}

static void
update_unread_items_count(size_t index, bool redraw)
{
	int new_unread_count = get_unread_items_count(feeds[index]->link);
	if (feeds[index]->unread_count != new_unread_count) {
		feeds[index]->unread_count = new_unread_count;
		if ((redraw == true) && (index >= view_min) && (index <= view_max)) {
			feed_expose(index);
		}
	}
}

static void
reload_current_feed(void)
{
	status_write("Loading %s", feeds[view_sel]->link->ptr);

	if (update_feed(feeds[view_sel]->link) == false) {
		status_write("Failed to load %s", feeds[view_sel]->link->ptr);
		return;
	}

	update_unread_items_count(view_sel, true);
	status_clean();
}

static void
reload_all_feeds(void)
{
	size_t errors = 0;
	const struct string *failed_feed;

	for (size_t i = 0; i < feeds_count; ++i) {
		if (feeds[i]->link == NULL) {
			continue;
		}
		status_write("(%d/%d) Loading %s", i + 1, feeds_count, feeds[i]->link->ptr);
		if (update_feed(feeds[i]->link) == true) {
			update_unread_items_count(i, true);
		} else {
			failed_feed = feeds[i]->link;
			++errors;
		}
	}

	if (errors == 0) {
		status_clean();
	} else if (errors == 1) {
		status_write("Failed to update %s", failed_feed->ptr);
	} else {
		status_write("Failed to update %u feeds.", errors);
	}
}

static void
view_select_next(void)
{
	view_select(view_sel + 1);
}

static void
view_select_prev(void)
{
	view_select((view_sel == 0) ? (0) : (view_sel - 1));
}

static void
view_select_first(void)
{
	view_select(0);
}

static void
view_select_last(void)
{
	view_select((feeds_count == 0) ? (0) : (feeds_count - 1));
}

static void
redraw_feeds_windows(void)
{
	clear();
	refresh();
	status_update();
	view_max = view_min + (list_menu_height - 1);
	if (view_max < view_sel) {
		view_max = view_sel;
		view_min = view_max - (list_menu_height - 1);
	}
	show_feeds();
}

static void
set_feeds_input_handlers(void)
{
	reset_input_handlers();
	set_input_handler(INPUT_SELECT_NEXT, &view_select_next);
	set_input_handler(INPUT_SELECT_PREV, &view_select_prev);
	set_input_handler(INPUT_SELECT_FIRST, &view_select_first);
	set_input_handler(INPUT_SELECT_LAST, &view_select_last);
	set_input_handler(INPUT_RELOAD, &reload_current_feed);
	set_input_handler(INPUT_RELOAD_ALL, &reload_all_feeds);
	set_input_handler(INPUT_RESIZE, &redraw_feeds_windows);
}

void
enter_feeds_menu_loop(void)
{
	if (obtain_feeds_of_section(cfg.global_section_name, &feeds, &feeds_count) == false) {
		return;
	}

	view_sel = 0;
	view_min = 0;
	view_max = list_menu_height - 1;

	redraw_feeds_windows();

	set_feeds_input_handlers();

	int destination;
	while (true) {
		destination = handle_input();
		if (destination == INPUT_QUIT_SOFT || destination == INPUT_QUIT_HARD) {
			break;
		}

		destination = enter_items_menu_loop(feeds[view_sel]->link);

		if (destination == INPUT_QUIT_SOFT) {
			status_clean();
			set_feeds_input_handlers();
			update_unread_items_count(view_sel, false);
			redraw_feeds_windows();
		} else if (destination == INPUT_QUIT_HARD) {
			break;
		}
	}
}
