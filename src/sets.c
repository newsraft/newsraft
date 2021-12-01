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

void
free_sets(void)
{
	free_tags();
	if (sets == NULL) return;
	for (size_t i = 0; i < sets_count; ++i) {
		free_string(sets[i].name);
		free_string(sets[i].link);
		free_string(sets[i].tags);
		free_set_condition(sets[i].cond);
	}
	free(sets);
}

static int
parse_sets_file(void) {
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

	// this is line-by-line file processing loop:
	// one iteration of loop == one processed line
	while (1) {

		// get first non-whitespace character
		do { c = fgetc(f); } while (c == ' ' || c == '\t' || c == '\n');

		if (c == '#') {
			// skip a comment line
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == '\n') {
				continue;
			} else {
				break; // quit on end of file
			}
		}

		if (c == EOF) break; // quit on end of file

		set_index = sets_count++;
		sets = realloc(sets, sizeof(struct set_line) * sets_count);
		if (sets == NULL) {
			error = 1;
			break;
		}
		sets[set_index].name = NULL;
		sets[set_index].link = NULL;
		sets[set_index].tags = NULL;
		sets[set_index].cond = NULL;
		sets[set_index].unread_count = 0;

		word_len = 0;
		if (c == '@') { // line is decoration

			while (1) {
				c = fgetc(f);
				if (c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
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
				if (c == ' ' || c == '\t') {
					/* avoid any whitespace in tags expression */
					continue;
				}
				if (c == '"' || c == '\n' || c == EOF) {
					word[word_len] = '\0';
					break;
				}
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
					if (c == '"' || c == '\n' || c == EOF) {
						word[word_len] = '\0';
						break;
					}
					word[word_len++] = c;
				}
				sets[set_index].name = create_string(word, word_len);
				if (sets[set_index].name == NULL) {
					error = 1;
					break;
				}
				if (c == '"') {
					/* skip everything in the line after closing double quote */
					do { c = fgetc(f); } while (c != '\n' && c != EOF);
				}
			}

		} else { // line is feed

			while (1) {
				word[word_len++] = c;
				c = fgetc(f);
				if (c == ' ' || c == '\t' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
			}
			sets[set_index].link = create_string(word, word_len);
			if (sets[set_index].link == NULL) {
				error = 1;
				break;
			}
			while (c == ' ' || c == '\t') { c = fgetc(f); }
			// process name
			if (c == '"') {
				word_len = 0;
				while (1) {
					c = fgetc(f);
					if (c == '"' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
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
					if (c == ' ' || c == '\t' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
				}
				if (tag_feed(word, sets[set_index].link) != 0) {
					error = 1;
					break;
				}
				while (c == ' ' || c == '\t') { c = fgetc(f); }
			}

		}
		if (c == EOF) break;
	}
	fclose(f);
	if (error != 0) {
		if (error == 1) {
			fprintf(stderr, "Not enough memory for parsing feeds file!\n");
		}
	}
	return error;
}

int
load_sets(void)
{
	if (parse_sets_file() != 0) {
		fprintf(stderr, "Failed to load sets from file!\n");
		free_sets();
		return 1; // failure
	}

	bool error = false;

	for (size_t i = 0; i < sets_count; ++i) {
		if ((sets[i].link == NULL) && (sets[i].tags == NULL)) {
			continue;
		}
		if (sets[i].link != NULL) {
			sets[i].cond = create_set_condition_for_feed(sets[i].link);
		} else if (sets[i].tags != NULL) {
			sets[i].cond = create_set_condition_for_filter(sets[i].tags);
		}
		if (sets[i].cond == NULL) {
			fprintf(stderr, "Error occurred on \"%s\" entry!\n", sets[i].tags != NULL ? sets[i].tags->ptr : sets[i].link->ptr);
			error = true;
		}
		sets[i].unread_count = get_unread_items_count(sets[i].cond);
	}

	if (error == true) {
		fprintf(stderr, "Was not able to create conditions for entries!\n");
		free_sets();
		return 1;
	}

	view_sel = SIZE_MAX;
	// put first non-decorative set in selection
	for (size_t i = 0; i < sets_count; ++i) {
		if (sets[i].tags != NULL || sets[i].link != NULL) {
			view_sel = i;
			break;
		}
	}

	if (view_sel == SIZE_MAX) {
		fprintf(stderr, "None of feeds loaded!\n");
		free_sets();
		return 1; // failure
	}

	return 0; // success
}

static void
set_expose(size_t index)
{
	werase(sets[index].window);
	print_set_format(index, &sets[index]);
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

	// perform boundary check
	if (new_sel >= sets_count) {
		if (sets_count == 0) {
			return;
		}
		new_sel = sets_count - 1;
	}

	if (sets[new_sel].link == NULL && sets[new_sel].tags == NULL) {
		// skip decorations
		size_t temp_new_sel = new_sel;
		if (new_sel > view_sel) {
			for (size_t j = view_sel; j < sets_count; ++j) {
				if (sets[j].link != NULL || sets[j].tags != NULL) {
					temp_new_sel = j;
					if (temp_new_sel >= new_sel) break;
				}
			}
		} else if (new_sel < view_sel) {
			for (size_t j = view_sel; ; --j) {
				if (sets[j].link != NULL || sets[j].tags != NULL) {
					temp_new_sel = j;
					if (j <= new_sel) break;
				}
				if (j == 0) break;
			}
		}
		new_sel = temp_new_sel;
	}

	if ((sets[new_sel].link == NULL && sets[new_sel].tags == NULL) || new_sel == view_sel) {
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
		size_t old_sel = view_sel;
		view_sel = new_sel;
		set_expose(old_sel);
		set_expose(view_sel);
	}
}

static void
set_reload_feed(struct set_line *set, size_t index)
{
	status_write("Loading %s", set->link->ptr);
	if (update_feed(set->link) == 0) {
		size_t new_unread_count = get_unread_items_count(set->cond);
		if (set->unread_count != new_unread_count) {
			set->unread_count = new_unread_count;
			set_expose(index);
		}
		status_clean();
	} else {
		status_write("Failed to update %s", set->link->ptr);
	}
}

static void
set_reload_filter(struct set_line *set, size_t index)
{
	(void)index;
	if (sets[index].cond == NULL) {
		status_write("There was a problem generating search condition for that set!");
		return;
	}
	size_t feed_unread_count;
	size_t errors = 0;
	/* Here we trying to reload all feed urls related to this filter. */
	for (size_t i = 0; i < set->cond->urls_count; ++i) {
		status_write("Loading %s", set->cond->urls[i]->ptr);
		if (update_feed(set->cond->urls[i]) == 0) {
			for (size_t j = 0; j < sets_count; ++j) {
				if ((sets[j].link != NULL) && (strcmp(sets[j].link->ptr, set->cond->urls[i]->ptr) == 0)) {
					feed_unread_count = get_unread_items_count(sets[j].cond);
					if (sets[j].unread_count != feed_unread_count) {
						sets[j].unread_count = feed_unread_count;
						if ((j >= view_min) && (j <= view_max)) {
							set_expose(j);
						}
					}
					break;
				}
			}
		} else {
			++errors;
		}
	}
	if (errors == 0) {
		size_t new_unread_count = get_unread_items_count(set->cond);
		if (set->unread_count != new_unread_count) {
			set->unread_count = new_unread_count;
			set_expose(index);
		}
		status_clean();
	} else if (errors == 1) {
		status_write("Failed to update 1 feed.");
	} else {
		status_write("Failed to update %u feeds.", errors);
	}
}

static void
reload_current_set(void)
{
	struct set_line *set = &(sets[view_sel]);
	if (set->link != NULL) {
		set_reload_feed(set, view_sel);
	} else if (set->tags != NULL) {
		set_reload_filter(set, view_sel);
	}
}

static void
reload_all_feeds(void)
{
	for (size_t i = 0; i < sets_count; ++i) {
		if (sets[i].link != NULL) {
			set_reload_feed(&sets[i], i);
		}
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
			status_write("There was a problem generating search condition for that set!");
			continue;
		}

		destination = enter_items_menu_loop(sets[view_sel].cond);

		if (destination == INPUT_QUIT_SOFT) { /* stay in sets menu */
			set_sets_input_handlers();
			sets[view_sel].unread_count = get_unread_items_count(sets[view_sel].cond);
			redraw_sets_windows();
		}

		if (destination == INPUT_QUIT_HARD) { /* exit the program */
			break;
		}
	}
}
