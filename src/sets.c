#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

static struct set_line *sets = NULL;
static size_t sets_count = 0;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"d", {.i = 0}},
	{L't', L"s", {.s = NULL}},
};

void
free_sets(void)
{
	if (sets == NULL) return;
	for (size_t i = 0; i < sets_count; ++i) {
		free_string(sets[i].name);
		free_string(sets[i].link);
	}
	free(sets);
}

// On success returns 0.
// On failure returns non-zero.
static inline bool
parse_sets_file(void)
{
	const char *path = get_feeds_path();
	if (path == NULL) {
		// Error message is written by get_feeds_path().
		return false;
	}
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "Could not open feeds file!\n");
		return false;
	}
	size_t set_index, word_len;
	char c, word[1000];
	struct set_line *temp;

	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (1) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (ISWHITESPACE(c));

		if (c == '#') {
			// Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == '\n') {
				continue;
			} else {
				break; // Quit on end of file.
			}
		} else if (c == EOF) {
			break; // Quit on end of file.
		}

		set_index = sets_count++;
		temp = realloc(sets, sizeof(struct set_line) * sets_count);
		if (temp == NULL) { goto error; }
		sets = temp;
		sets[set_index].name = NULL;
		sets[set_index].link = NULL;
		sets[set_index].unread_count = 0;

		word_len = 0;
		while (1) {
			word[word_len++] = c;
			c = fgetc(f);
			if (ISWHITESPACE(c) || c == EOF) { break; }
		}
		sets[set_index].link = crtas(word, word_len);
		if (sets[set_index].link == NULL) { goto error; }
		sets[set_index].unread_count = get_unread_items_count(sets[set_index].link);
		while (ISWHITESPACEEXCEPTNEWLINE(c)) { c = fgetc(f); }
		// process name
		if (c == '"') {
			word_len = 0;
			while (1) {
				c = fgetc(f);
				if (c == '"' || c == '\n' || c == EOF) { break; }
				word[word_len++] = c;
			}
			sets[set_index].name = crtas(word, word_len);
			if (sets[set_index].name == NULL) { goto error; }
			if (c == '"') {
				c = fgetc(f);
			}
			while (ISWHITESPACEEXCEPTNEWLINE(c)) { c = fgetc(f); }
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

	fclose(f);
	return true;
error:
	fclose(f);
	return false;
}

// On success returns 0.
// On failure returns non-zero.
bool
load_sets(void)
{
	if (parse_sets_file() == false) {
		fprintf(stderr, "Failed to load sets from file!\n");
		free_sets();
		return false;
	}

	if (sets_count == 0) {
		fprintf(stderr, "None of feeds loaded!\n");
		free_sets();
		return false;
	}

	return true;
}

// Returns most sensible string for set line name.
static inline char *
set_image(const struct set_line *set)
{
	if (set->name != NULL) {
		return set->name->ptr;
	} else {
		if (set->link != NULL) {
			return set->link->ptr;
		} else {
			return "";
		}
	}
}

static void
set_expose(size_t index)
{
	werase(sets[index].window);
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = sets[index].unread_count;
	fmt_args[2].value.s = set_image(&(sets[index]));
	mvwaddnwstr(sets[index].window, 0, 0, do_format(cfg.menu_set_entry_format, fmt_args, COUNTOF(fmt_args)), list_menu_width);
	mvwchgat(sets[index].window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(sets[index].window);
}

static void
show_sets(void)
{
	for (size_t i = view_min, j = 0; i < sets_count && i <= view_max; ++i, ++j) {
		sets[i].window = get_list_entry_by_index(j);
		set_expose(i);
	}
}

static void
view_select(size_t i)
{
	size_t new_sel = i;

	if (new_sel >= sets_count) {
		// Don't check if sets_count is zero because program
		// won't even get here when not a single set loaded.
		new_sel = sets_count - 1;
	}

	if (new_sel == view_sel) {
		return;
	}

	if (new_sel > view_max) {
		view_min = new_sel - (list_menu_height - 1);
		view_max = new_sel;
		view_sel = new_sel;
		show_sets();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + (list_menu_height - 1);
		view_sel = new_sel;
		show_sets();
	} else {
		mvwchgat(sets[view_sel].window, 0, 0, -1, A_NORMAL, 0, NULL);
		wrefresh(sets[view_sel].window);
		view_sel = new_sel;
		mvwchgat(sets[view_sel].window, 0, 0, -1, A_REVERSE, 0, NULL);
		wrefresh(sets[view_sel].window);
	}
}

static void
update_unread_items_count(struct set_line *set, size_t index, bool redraw)
{
	int new_unread_count = get_unread_items_count(set->link);
	if (set->unread_count != new_unread_count) {
		set->unread_count = new_unread_count;
		if ((redraw == true) && (index >= view_min) && (index <= view_max)) {
			set_expose(index);
		}
	}
}

static void
reload_current_set(void)
{
	status_write("Loading %s", sets[view_sel].link->ptr);

	if (update_feed(sets[view_sel].link) == false) {
		status_write("Failed to load %s", sets[view_sel].link->ptr);
		return;
	}

	update_unread_items_count(&sets[view_sel], view_sel, true);
	status_clean();
}

static void
reload_all_feeds(void)
{
	size_t errors = 0;
	const struct string *failed_feed;

	for (size_t i = 0; i < sets_count; ++i) {
		if (sets[i].link == NULL) {
			continue;
		}
		status_write("(%d/%d) Loading %s", i + 1, sets_count, sets[i].link->ptr);
		if (update_feed(sets[i].link) == true) {
			update_unread_items_count(&sets[i], i, true);
		} else {
			failed_feed = sets[i].link;
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
	view_select((sets_count == 0) ? (0) : (sets_count - 1));
}

static void
redraw_sets_windows(void)
{
	clear();
	refresh();
	status_update();
	view_max = view_min + (list_menu_height - 1);
	if (view_max < view_sel) {
		view_max = view_sel;
		view_min = view_max - (list_menu_height - 1);
	}
	show_sets();
}

static void
set_sets_input_handlers(void)
{
	reset_input_handlers();
	set_input_handler(INPUT_SELECT_NEXT, &view_select_next);
	set_input_handler(INPUT_SELECT_PREV, &view_select_prev);
	set_input_handler(INPUT_SELECT_FIRST, &view_select_first);
	set_input_handler(INPUT_SELECT_LAST, &view_select_last);
	set_input_handler(INPUT_RELOAD, &reload_current_set);
	set_input_handler(INPUT_RELOAD_ALL, &reload_all_feeds);
	set_input_handler(INPUT_RESIZE, &redraw_sets_windows);
}

void
enter_sets_menu_loop(void)
{
	view_sel = 0;
	view_min = 0;
	view_max = list_menu_height - 1;

	redraw_sets_windows();

	set_sets_input_handlers();

	int destination;
	while (1) {
		destination = handle_input();
		if (destination == INPUT_QUIT_SOFT || destination == INPUT_QUIT_HARD) {
			break;
		}

		destination = enter_items_menu_loop(sets[view_sel].link);

		if (destination == INPUT_QUIT_SOFT) {
			status_clean();
			set_sets_input_handlers();
			update_unread_items_count(&sets[view_sel], view_sel, false);
			redraw_sets_windows();
		} else if (destination == INPUT_QUIT_HARD) {
			break;
		}
	}
}
