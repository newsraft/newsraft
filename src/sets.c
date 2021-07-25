#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

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
	char *path = get_conf_file_path("feeds");
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

		// quit on end of file
		if (c == EOF) break;

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
		sets[set_index].is_marked = false;
		sets[set_index].is_unread = false;

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
					do { c = fgetc(f); } while (c == ' ' || c == '\t');
				}
			}

		} else { // line is feed

			while (1) {
				word[word_len++] = c;
				c = fgetc(f);
				if (c == ' ' || c == '\t' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
			}
			sets[set_index].link = create_string(word, word_len);
			sets[set_index].is_marked = is_feed_marked(sets[set_index].link);
			sets[set_index].is_unread = is_feed_unread(sets[set_index].link);
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

// return most sensible string for set line
static char *
set_image(struct set_line *set)
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
	struct set_line *set = &sets[index];
	if (config_menu_show_number == true) {
		if (set->link != NULL ||
		    set->tags != NULL ||
		    config_menu_show_decoration_number == true)
		{
			mvwprintw(set->window, 0, 0, "%3d", index + 1);
		}
		mvwprintw(set->window, 0, 5, set->is_marked ? "M" : " ");
		mvwprintw(set->window, 0, 6, set->is_unread ? "N" : " ");
		mvwprintw(set->window, 0, 9, "%s", set_image(set));
	} else {
		mvwprintw(set->window, 0, 2, set->is_marked ? "M" : " ");
		mvwprintw(set->window, 0, 3, set->is_unread ? "N" : " ");
		mvwprintw(set->window, 0, 6, "%s", set_image(set));
	}
	mvwchgat(set->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(set->window);
}

static void
show_sets(void)
{
	for (size_t i = view_min, j = 0; i < sets_count && i <= view_max; ++i, ++j) {
		sets[i].window = newwin(1, COLS, j, 0);
		set_expose(i);
	}
}

void
hide_sets(void)
{
	for (size_t i = view_min; i < sets_count && i <= view_max; ++i) {
		if (sets[i].window != NULL) {
			delwin(sets[i].window);
		}
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

	// skip decorations
	if (sets[new_sel].link == NULL && sets[new_sel].tags == NULL) {
		size_t temp_new_sel = new_sel;
		if (new_sel > view_sel) {
			for (size_t j = view_sel; j < sets_count; ++j) {
				if (sets[j].link != NULL || sets[j].tags != NULL) {
					temp_new_sel = j;
					if (j >= new_sel) break;
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
		hide_sets();
		view_min = new_sel - LINES + 2;
		view_max = new_sel;
		view_sel = new_sel;
		show_sets();
	} else if (new_sel < view_min) {
		hide_sets();
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
set_reload(size_t index)
{
	struct set_line *set = &sets[index];
	if (set->link != NULL) {
		status_write("[loading] %s", set_image(set));
		struct string *buf = feed_download(set->link->ptr);
		if (buf == NULL) {
			return;
		} else if (buf->ptr == NULL) {
			free(buf);
			return;
		}
		if (feed_process(buf, set->link) == 0) {
			status_clean();
			bool unread_status = is_feed_unread(set->link);
			if (set->is_unread != unread_status) {
				set->is_unread = unread_status;
				set_expose(index);
			}
		}
		free_string(buf);
	} else {
		//under construction
		status_write("[under construction] can't reload filter");
	}
}

static void
all_reload(void)
{
	//under construction
	return;
}

static enum menu_dest
menu_feeds(void)
{
	int ch, q, i;
	char cmd[7];
	while (1) {
		ch = input_wgetch();
		if      (ch == 'j' || ch == KEY_DOWN)                            { view_select(view_sel + 1); }
		else if ((ch == 'k' || ch == KEY_UP) && (view_sel != 0))         { view_select(view_sel - 1); }
		else if (ch == config_key_download)                              { set_reload(view_sel); }
		else if (ch == config_key_download_all)                          { all_reload(); }
		else if ((ch == 'g' && input_wgetch() == 'g') || ch == KEY_HOME) { view_select(0); }
		else if ((ch == 'G' || ch == KEY_END) && (sets_count != 0))      { view_select(sets_count - 1); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = input_wgetch();
				if (!isdigit(ch)) {
					i = atoi(cmd);
					if (ch == 'j' || ch == KEY_DOWN) {
						view_select(view_sel + i);
					} else if ((ch == 'k' || ch == KEY_UP) && ((size_t)i <= view_sel)) {
						view_select(view_sel - i);
					} else if ((ch == 'G') && (i != 0)) {
						view_select(i - 1);
					}
					break;
				}
			}
		}
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_ITEMS; }
		else if (ch == config_key_soft_quit || ch == config_key_hard_quit)      { return MENU_QUIT; }
	}
}

int
run_sets_menu(void)
{
	view_min = 0;
	view_max = LINES - 2;
	clear();
	refresh();
	show_sets();

	int dest;
	bool status_cond;
	struct set_statement *st;
	while ((dest = menu_feeds()) != MENU_QUIT) {
		if ((st = create_set_statement(&sets[view_sel])) != NULL) {
			dest = run_items_menu(st);
			free(st->urls);
			free_string(st->db_cmd);
			free(st);
			if (dest == MENU_FEEDS) {
				clear();
				refresh();
				show_sets();
				if (sets[view_sel].link != NULL) {
					status_cond = is_feed_unread(sets[view_sel].link);
					if (status_cond != sets[view_sel].is_unread) {
						sets[view_sel].is_unread = status_cond;
						set_expose(view_sel);
					}
					status_cond = is_feed_marked(sets[view_sel].link);
					if (status_cond != sets[view_sel].is_marked) {
						sets[view_sel].is_marked = status_cond;
						set_expose(view_sel);
					}
				}
			} else if (dest == MENU_QUIT) {
				break;
			}
		} else {
			status_write("[empty] %s", set_image(&sets[view_sel]));
		}
	}

	return 0;
}
