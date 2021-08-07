#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

static struct set_line *sets = NULL;
static size_t sets_count = 0;
static size_t view_sel;
// [view_min; view_max] is index range of displayed sets
static size_t view_min;
static size_t view_max;

void
free_sets(void)
{
	free_tags();
	if (sets == NULL) return;
	for (size_t i = 0; i < sets_count; ++i) {
		free_string(sets[i].name);
		free_string(sets[i].link);
		free_string(sets[i].tags);
	}
	free(sets);
}

static int
parse_sets_file(void) {
	char *path = get_feeds_path();
	if (path == NULL) {
		fprintf(stderr, "could not find feeds file!\n");
		return 1;
	}
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) {
		fprintf(stderr, "could not open feeds file!\n");
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
			fprintf(stderr, "reallocation for sets failed\n");
			error = 1;
			break;
		}
		sets[set_index].name = NULL;
		sets[set_index].link = NULL;
		sets[set_index].tags = NULL;
		sets[set_index].unread_count = 0;

		word_len = 0;
		if (c == '@') { // line is decoration

			while (1) {
				c = fgetc(f);
				if (c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
				word[word_len++] = c;
			}
			sets[set_index].name = create_string(word, word_len);

		} else if (c == '!') { // line is filter

			while (1) {
				c = fgetc(f);
				if (c == ' ' || c == '\t') {
					/* tags expression MUST NOT contain any whitespace! */
					continue;
				}
				if (c == '"' || c == '\n' || c == EOF) {
					word[word_len] = '\0';
					break;
				}
				word[word_len++] = c;
			}
			sets[set_index].tags = create_string(word, word_len);
			if (c == '"') {
				word_len = 0;
				while (1) {
					c = fgetc(f);
					if (c == '"' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
					word[word_len++] = c;
				}
				sets[set_index].name = create_string(word, word_len);
				if (c == '"') {
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
			sets[set_index].unread_count = get_unread_items_count_of_feed(sets[set_index].link);
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
				tag_feed(word, sets[set_index].link);
				while (c == ' ' || c == '\t') { c = fgetc(f); }
			}

		}
		if (c == EOF) break;
	}
	fclose(f);
	return error;
}

int
load_sets(void)
{
	if (parse_sets_file() != 0) {
		fprintf(stderr, "failed to load sets from file!\n");
		free_sets();
		return 1; // failure
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
		fprintf(stderr, "none of feeds loaded!\n");
		free_sets();
		return 1; // failure
	}

	debug_tags_summary();

	return 0; // success
}

static void
set_expose(size_t index)
{
	struct set_line *set = &sets[index];
	werase(set->window);
	print_set_format(index, set);
	mvwchgat(set->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(set->window);
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
		view_min = new_sel - LINES + 2;
		view_max = new_sel;
		view_sel = new_sel;
		show_sets();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + LINES - 2;
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
	status_write("[loading] %s", set->link->ptr);
	if (feed_process(set->link) == 0) {
		status_clean();
		size_t new_unread_count = get_unread_items_count_of_feed(set->link);
		if (set->unread_count != new_unread_count) {
			set->unread_count = new_unread_count;
			set_expose(index);
		}
	}
}

static void
set_reload_filter(struct set_line *set, size_t index)
{
	(void)index;
	struct set_condition *st;
	if ((st = create_set_condition(set)) == NULL) {
		/* error message is written to status by create_set_condition */
		return;
	}
	/* Here we trying to reload all feed urls related to this filter. */
	for (size_t i = 0; i < st->urls_count; ++i) {
		status_write("[loading] %s", st->urls[i]->ptr);
		if (feed_process(st->urls[i]) == 0) {
			status_clean();
			// TODO: change unread status of updated feed somehow
			/*bool unread_status = is_feed_unread(st->urls[i]);*/
			/*if (set->is_unread != unread_status) {*/
				/*set->is_unread = unread_status;*/
				/*set_expose(index);*/
			/*}*/
		}
	}
	free_set_condition(st);
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
redraw_sets_by_resize(void)
{
	view_max = view_min + LINES - 2;
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
	set_input_handler(INPUT_RESIZE, &redraw_sets_by_resize);
}

void
enter_sets_menu_loop(void)
{
	view_min = 0;
	view_max = LINES - 2;
	set_sets_input_handlers();
	clear();
	refresh();
	show_sets();

	int dest;
	size_t new_unread_count;
	struct set_condition *st;
	while (1) {
		dest = handle_input();
		if (dest == INPUT_SOFT_QUIT || dest == INPUT_HARD_QUIT) {
			break;
		}
		if ((st = create_set_condition(&sets[view_sel])) == NULL) {
			/* error message is written to status by create_set_condition */
			continue;
		}
		dest = enter_items_menu_loop(st);
		free_set_condition(st);
		if (dest == INPUT_HARD_QUIT) { /* exit the program */
			break;
		} else if (dest == INPUT_SOFT_QUIT) { /* return to sets menu again */
			set_sets_input_handlers();
			clear();
			refresh();
			show_sets();
			if (sets[view_sel].link != NULL) {
				/* check if feed changed its read state */
				new_unread_count = get_unread_items_count_of_feed(sets[view_sel].link);
				if (sets[view_sel].unread_count != new_unread_count) {
					sets[view_sel].unread_count = new_unread_count;
					set_expose(view_sel);
				}
			} else if (sets[view_sel].tags != NULL) {
				/* TODO check if filter changed its read state */
			}
		}
	}
}
