#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

static struct set_line *sets = NULL;
static size_t sets_count = 0;
static size_t feeds_count = 0;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static struct format_arg fmt_args[] = {
	{'n', 'd', {.i = 0}},
	{'u', 'd', {.i = 0}},
	{'t', 's', {.s = NULL}},
};

void
free_sets(void)
{
	if (sets == NULL) return;
	for (size_t i = 0; i < sets_count; ++i) {
		free_string((struct string *)sets[i].name);
		free_string((struct string *)sets[i].link);
		free_string((struct string *)sets[i].tags);
		free_set_condition(sets[i].cond);
	}
	free(sets);
}

// On success returns 0.
// On failure returns non-zero.
static inline int
parse_sets_file(struct feed_tag **head_tag_ptr) {
	char *path = get_feeds_path();
	if (path == NULL) {
		fprintf(stderr, "Could not find feeds file!\n");
		return 1;
	}
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) {
		fprintf(stderr, "Could not open feeds file!\n");
		return 1;
	}
	size_t set_index, word_len;
	int error = 0;
	char c, word[1000];
	struct set_line *temp;

	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (1) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (c == ' ' || c == '\t' || c == '\n');

		if (c == '#') {
			// Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == '\n') {
				continue;
			} else {
				// Quit on end of file.
				break;
			}
		} else if (c == EOF) {
			// Quit on end of file.
			break;
		}

		set_index = sets_count++;
		temp = realloc(sets, sizeof(struct set_line) * sets_count);
		if (temp == NULL) {
			error = 1;
			break;
		}
		sets = temp;
		sets[set_index].name = NULL;
		sets[set_index].link = NULL;
		sets[set_index].tags = NULL;
		sets[set_index].cond = NULL;
		sets[set_index].unread_count = 0;

		word_len = 0;
		if (c == '@') { // line is decoration

			while (1) {
				c = fgetc(f);
				if (c == '\n' || c == EOF) { break; }
				word[word_len++] = c;
			}
			sets[set_index].name = create_string(word, word_len);
			if (sets[set_index].name == NULL) {
				error = 1;
				break;
			}

		} else if (c == '!') { // line is filter

			while (1) {
				c = fgetc(f);
				if (c == ' ' || c == '\t') { continue; } // skip whitespace
				if (c == '"' || c == '\n' || c == EOF) { break; }
				word[word_len++] = c;
			}
			sets[set_index].tags = create_string(word, word_len);
			if (sets[set_index].tags == NULL) {
				error = 1;
				break;
			}
			if (c == '"') {
				/* double quote shows beginning of the filter name */
				word_len = 0;
				while (1) {
					c = fgetc(f);
					if (c == '"' || c == '\n' || c == EOF) { break; }
					word[word_len++] = c;
				}
				sets[set_index].name = create_string(word, word_len);
				if (sets[set_index].name == NULL) {
					error = 1;
					break;
				}
			}

		} else { // line is feed

			while (1) {
				word[word_len++] = c;
				c = fgetc(f);
				if (c == ' ' || c == '\t' || c == '\n' || c == EOF) { break; }
			}
			sets[set_index].link = create_string(word, word_len);
			if (sets[set_index].link == NULL) {
				error = 1;
				break;
			}
			while (c == ' ' || c == '\t') { c = fgetc(f); } // skip whitespace
			// process name
			if (c == '"') {
				word_len = 0;
				while (1) {
					c = fgetc(f);
					if (c == '"' || c == '\n' || c == EOF) { break; }
					word[word_len++] = c;
				}
				sets[set_index].name = create_string(word, word_len);
				if (sets[set_index].name == NULL) {
					error = 1;
					break;
				}
				if (c == '"') {
					do { c = fgetc(f); } while (c == ' ' || c == '\t');
				}
			}
			// process tags
			while (c != '\n' && c != EOF) {
				word_len = 0;
				while (1) {
					word[word_len++] = c;
					c = fgetc(f);
					if (c == ' ' || c == '\t' || c == '\n' || c == EOF) { break; }
				}
				word[word_len] = '\0';
				if (tag_feed(head_tag_ptr, word, word_len, sets[set_index].link) != 0) {
					error = 1;
					break;
				}
				while (c == ' ' || c == '\t') { c = fgetc(f); }
			}

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
	if (error != 0) {
		if (error == 1) {
			fprintf(stderr, "Not enough memory for parsing sets file!\n");
		}
	}
	return error;
}

// On success returns 0.
// On failure returns non-zero.
int
load_sets(void)
{
	struct feed_tag *head_tag = NULL;

	if (parse_sets_file(&head_tag) != 0) {
		fprintf(stderr, "Failed to load sets from file!\n");
		free_sets();
		free_tags(head_tag);
		return 1;
	}

	bool error = false;

	for (size_t i = 0; i < sets_count; ++i) {
		if ((sets[i].link == NULL) && (sets[i].tags == NULL)) {
			continue;
		}
		if (sets[i].link != NULL) {
			sets[i].cond = create_set_condition_for_feed(sets[i].link);
			++feeds_count;
		} else if (sets[i].tags != NULL) {
			sets[i].cond = create_set_condition_for_filter(head_tag, sets[i].tags);
		}
		if (sets[i].cond == NULL) {
			// Error message is written by "create_set_condition_for_feed"
			// or "create_set_condition_for_filter". No worries!
			error = true;
			break;
		}
		sets[i].unread_count = get_unread_items_count(sets[i].cond);
	}

	// We don't need tags anymore because all conditions are already created.
	free_tags(head_tag);

	if (error == true) {
		fprintf(stderr, "Was not able to create conditions for entries!\n");
		free_sets();
		return 1;
	}

	view_sel = SIZE_MAX;
	// put first non-decorative set in selection
	for (size_t i = 0; i < sets_count; ++i) {
		if (sets[i].cond != NULL) {
			view_sel = i;
			break;
		}
	}

	if (view_sel == SIZE_MAX) {
		fprintf(stderr, "None of feeds loaded!\n");
		free_sets();
		return 1;
	}

	return 0;
}

// Returns most sensible string for set line name.
static char *
set_image(const struct set_line *set)
{
	if (set->name != NULL) {
		return set->name->ptr;
	} else {
		if (set->link != NULL) {
			return set->link->ptr;
		} else {
			if (set->tags != NULL) {
				return set->tags->ptr;
			} else {
				return "";
			}
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
	wprintw(sets[index].window, "%s", do_format(config_menu_set_entry_format, fmt_args, LENGTH(fmt_args)));
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
		if (sets_count == 0) {
			return;
		}
		new_sel = sets_count - 1;
	}

	if (sets[new_sel].cond == NULL) {
		// If set's condition is NULL, then it is a decoration.
		// Here we make sure that user never selects decorations.
		size_t temp_new_sel = new_sel;
		if (new_sel > view_sel) {
			for (size_t j = view_sel; j < sets_count; ++j) {
				if (sets[j].cond != NULL) {
					temp_new_sel = j;
					if (temp_new_sel >= new_sel) break;
				}
			}
		} else if (new_sel < view_sel) {
			for (size_t j = view_sel; ; --j) {
				if (sets[j].cond != NULL) {
					temp_new_sel = j;
					if (j <= new_sel) break;
				}
				if (j == 0) break;
			}
		}
		new_sel = temp_new_sel;
	}

	if ((sets[new_sel].cond == NULL) || (new_sel == view_sel)) {
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
update_unread_items_count(struct set_line *set, size_t index)
{
	int new_unread_count = get_unread_items_count(set->cond);
	if (set->unread_count != new_unread_count) {
		set->unread_count = new_unread_count;
		if ((index >= view_min) && (index <= view_max)) {
			set_expose(index);
		}
	}
}

static void
update_unread_items_count_recursively(struct set_line *set, size_t index)
{
	if (set->tags != NULL) { // set is filter
		// Here we are trying to update unread items count of
		// all sets (feeds and filters) related to this filter.
		// Feed is considered related to some filter if its link
		// is equal to one of the filter's URLs.
		// Filter is considered related to some filter if it has
		// URL that is equal to one of the another filter's URLs.
		for (size_t i = 0; i < set->cond->urls_count; ++i) {
			for (size_t j = 0; j < sets_count; ++j) {
				if (sets[j].link != NULL) { // sets[j] is feed
					if (sets[j].link == set->cond->urls[i]) {
						// Feed link matched one of the filters URLs, update sets[j]
						update_unread_items_count(&(sets[j]), j);
					}
				} else if (sets[j].tags != NULL) { // sets[j] is filter
					for (size_t k = 0; k < sets[j].cond->urls_count; ++k) {
						if (sets[j].cond->urls[k] == set->cond->urls[i]) {
							// Filters set and sets[j] have common URL, update sets[j]
							update_unread_items_count(&(sets[j]), j);
							break;
						}
					}
				}
			}
		}
	} else if (set->link != NULL) { // set is feed
		// Update unread items count of that set.
		update_unread_items_count(set, index);
		// Here we are trying to update unread items count of
		// all filters related to this feed.
		// Filter is considered related to some feed if it has
		// URL that is equal to the feed's link.
		for (size_t j = 0; j < sets_count; ++j) {
			if (sets[j].tags != NULL) { // sets[j] is filter
				for (size_t k = 0; k < sets[j].cond->urls_count; ++k) {
					if (sets[j].cond->urls[k] == set->link) {
						update_unread_items_count(&(sets[j]), j);
						break;
					}
				}
			}
		}
	}
}

static void
reload_current_set(void)
{
	size_t errors = 0;
	const struct string *failed_feed;

	for (size_t i = 0; i < sets[view_sel].cond->urls_count; ++i) {
		status_write("(%d/%d) Loading %s", i + 1, sets[view_sel].cond->urls_count, sets[view_sel].cond->urls[i]->ptr);
		if (update_feed(sets[view_sel].cond->urls[i]) != 0) {
			failed_feed = sets[view_sel].cond->urls[i];
			++errors;
		}
	}

	if (errors != sets[view_sel].cond->urls_count) {
		update_unread_items_count_recursively(&(sets[view_sel]), view_sel);
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
reload_all_feeds(void)
{
	size_t errors = 0;
	const struct string *failed_feed;

	for (size_t i = 0; i < sets_count; ++i) {
		if (sets[i].link == NULL) {
			continue;
		}
		status_write("(%d/%d) Loading %s", i + 1, feeds_count, sets[i].link->ptr);
		if (update_feed(sets[i].link) == 0) {
			update_unread_items_count_recursively(&(sets[i]), i);
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

void
resize_sets_global_action(void)
{
	view_min = view_sel;
	view_max = view_min + (list_menu_height - 1);
}

static void
redraw_sets_windows(void)
{
	clear();
	refresh();
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

		if (sets[view_sel].cond == NULL) {
			continue;
		}

		destination = enter_items_menu_loop(sets[view_sel].cond);

		if (destination == INPUT_QUIT_SOFT) { /* stay in sets menu */
			set_sets_input_handlers();

			update_unread_items_count_recursively(&(sets[view_sel]), view_sel);

			redraw_sets_windows();
		}

		if (destination == INPUT_QUIT_HARD) { /* exit the program */
			break;
		}
	}
}
