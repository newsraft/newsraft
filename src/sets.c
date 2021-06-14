#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static struct set_line *set_list;
static int set_count;
static char **tag_list;
static int tag_count;
/*
// example of the tag_list structure:
char tag_list[][] = {
	{"tag1_name", "feed1_url", "feed3_url", "feed4_url", NULL},
	{"tag2_name", "feed11_url", NULL},
	{"tag3_name", "feed16_url", "feed18_url", NULL},
}
int tag_count = 3;
*/
static int view_sel;
static int view_min;
static int view_max;

void
free_set_list(void)
{
	if (set_list == NULL) return;
	for (int i = 0; i < set_count; ++i) {
		if (set_list[i].name != NULL) free_string(&set_list[i].name);
		if (set_list[i].data != NULL) free_string(&set_list[i].data);
	}
	free(set_list);
}

static bool
is_feed_marked(struct string *url)
{
	if (url == NULL) return false;
	bool is_marked = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND marked = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed with this url is marked), say feed is marked
		if (sqlite3_step(res) == SQLITE_ROW) is_marked = true;
	} else {
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return is_marked;
}

static bool
is_feed_unread(struct string *url)
{
	if (url == NULL) return false;
	bool is_unread = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND unread = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed with this url is unread), say feed is unread
		if (sqlite3_step(res) == SQLITE_ROW) is_unread = true;
	} else {
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return is_unread;
}

int
load_set_list(void)
{
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

	int set_index, error = 0, word_len;
	char c, word[1000];
	set_list = NULL;
	set_count = 0;
	tag_list = NULL;
	tag_count = 0;
	view_sel = -1;

	while (1) {
		// get first character of the line
		c = fgetc(f);
		// skip white-space
		skip_chars(f, &c, " \t\n");
		// skip comment-line
		if (c == '#') { find_chars(f, &c, "\n"); continue; }
		if (c == EOF) break;
		set_index = set_count++;
		set_list = realloc(set_list, sizeof(struct set_line) * set_count);
		if (set_list == NULL) {
			error = 1; fprintf(stderr, "memory allocation for set list failed\n"); break;
		}
		set_list[set_index].type = EMPTY_ENTRY;
		set_list[set_index].name = NULL;
		set_list[set_index].data = NULL;
		set_list[set_index].is_marked = false;
		set_list[set_index].is_unread = false;
		word_len = 0;
		if (c == '@') {
			set_list[set_index].type = DECORATION_ENTRY;
			while (1) {
				c = fgetc(f);
				if (c == '\r' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
				word[word_len++] = c;
			}
			make_string(&set_list[set_index].name, word, word_len);
		} else if (c == '!') {
			set_list[set_index].type = FILTER_ENTRY;
			//under construction
			printf("yo this is filter!\n");
		} else {
			set_list[set_index].type = FEED_ENTRY;
			while (1) {
				word[word_len++] = c;
				c = fgetc(f);
				if (IS_WHITESPACE(c) || c == EOF) { word[word_len] = '\0'; break; }
			}
			make_string(&set_list[set_index].data, word, word_len);
			set_list[set_index].is_marked = is_feed_marked(set_list[set_index].data);
			set_list[set_index].is_unread = is_feed_unread(set_list[set_index].data);
			skip_chars(f, &c, " \t");
			if (c == '\n') continue;
			if (c == EOF) break;
			if (c == '"') {
				word_len = 0;
				while (1) {
					c = fgetc(f);
					if (c == '"' || c == '\r' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
					word[word_len++] = c;
				}
				make_string(&set_list[set_index].name, word, word_len);
			}
			// use some functions to put feed url to lists of all tags after feed name
			//while (tag = fget_word(f)) tag_url(set_list[set_index].data, tag);
		}
		// move to the end of line
		find_chars(f, &c, "\n");
	}
	fclose(f);

	if (error != 0) {
		fprintf(stderr, "there is some trouble in loading feeds!\n");
		free_set_list();
		return 1;
	}

	// put first non-decorative feed entry in selection
	for (set_index = 0; set_index < set_count; ++set_index) {
		if (set_list[set_index].data != NULL) {
			view_sel = set_index;
			break;
		}
	}

	if (view_sel == -1) {
		fprintf(stderr, "none of feeds loaded!\n");
		free_set_list();
		return 1;
	}

	return 0; // success
}

// return most sensible string for set line
static char *
set_image(struct set_line *set)
{
	if (set->name != NULL) {
		return set->name->ptr;
	} else {
		if (set->data != NULL) {
			return set->data->ptr;
		} else {
			return "";
		}
	}
}

static void
set_expose(int index)
{
	struct set_line *win = &set_list[index];
	if (config_menu_show_number == true) {
		if (win->data != NULL) mvwprintw(win->window, 0, 0, "%3d", index + 1);
		mvwprintw(win->window, 0, 5, win->is_marked ? "M" : " ");
		mvwprintw(win->window, 0, 6, win->is_unread ? "N" : " ");
		mvwprintw(win->window, 0, 9, "%s", set_image(win));
	} else {
		mvwprintw(win->window, 0, 2, win->is_marked ? "M" : " ");
		mvwprintw(win->window, 0, 3, win->is_unread ? "N" : " ");
		mvwprintw(win->window, 0, 6, "%s", set_image(win));
	}
	mvwchgat(win->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(win->window);
}

static void
show_sets(void)
{
	for (int i = view_min, j = 0; i < set_count && i < view_max; ++i, ++j) {
		set_list[i].window = newwin(1, COLS, j, 0);
		set_expose(i);
	}
}

void
hide_sets(void)
{
	for (int i = view_min; i < set_count && i < view_max; ++i) {
		if (set_list[i].window != NULL) {
			delwin(set_list[i].window);
		}
	}
}

static void
view_select(int i)
{
	int new_sel = i, j, old_sel;

	// perform boundary check
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= set_count) {
		new_sel = set_count - 1;
		if (new_sel < 0) return;
	}

	// skip decorations
	if (set_list[new_sel].data == NULL) {
		if (new_sel > view_sel) {
			for (j = new_sel + 1; (j < set_count) && (set_list[j].data == NULL); ++j);
			if (j < set_count) {
				new_sel = j;
			} else {
				for (j = view_sel; j < set_count; ++j) {
					if (set_list[j].data != NULL) new_sel = j;
				}
			}
		} else if (new_sel < view_sel) {
			for (j = new_sel - 1; (j > -1) && (set_list[j].data == NULL); --j);
			if (j > -1) {
				new_sel = j;
			} else {
				for (j = view_sel; j > -1; --j) {
					if (set_list[j].data != NULL) new_sel = j;
				}
			}
		}
	}

	if (set_list[new_sel].data != NULL && new_sel != view_sel) {
		if (new_sel >= view_max) {
			hide_sets();
			view_min += new_sel - view_sel;
			view_max += new_sel - view_sel;
			if (view_max > set_count) {
				view_max = set_count;
				view_min = view_max - LINES + 1;
			}
			view_sel = new_sel;
			show_sets();
		} else if (new_sel < view_min) {
			hide_sets();
			view_min -= view_sel - new_sel;
			view_max -= view_sel - new_sel;
			if (view_min < 0) {
				view_min = 0;
				view_max = LINES - 1;
			}
			view_sel = new_sel;
			show_sets();
		} else {
			old_sel = view_sel;
			view_sel = new_sel;
			set_expose(old_sel);
			set_expose(view_sel);
		}
	}
}

static void
set_reload(int index)
{
	struct set_line *set = &set_list[index];
	if (set->type == FEED_ENTRY) {
		status_write("[loading] %s", set_image(set));
		struct string *buf = feed_download(set->data->ptr);
		if (buf == NULL) return;
		if (buf->ptr == NULL) { free(buf); return; }
		if (feed_process(buf, set->data) == 0) {
			status_clean();
			bool unread_status = is_feed_unread(set->data);
			if (set->is_unread != unread_status) {
				set->is_unread = unread_status;
				set_expose(index);
			}
		}
		free_string(&buf);
	} else if (set->type == FILTER_ENTRY) {
		//under construction
		printf("yo reloading filter!\n");
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
	int ch, q;
	char cmd[7];
	while (1) {
		ch = input_wgetch();
		if      (ch == 'j' || ch == KEY_DOWN)                            { view_select(view_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)                              { view_select(view_sel - 1); }
		else if (ch == config_key_download)                              { set_reload(view_sel); }
		else if (ch == config_key_download_all)                          { all_reload(); }
		else if ((ch == 'g' && input_wgetch() == 'g') || ch == KEY_HOME) { view_select(0); }
		else if (ch == 'G' || ch == KEY_END)                             { view_select(set_count - 1); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = input_wgetch();
				if (!isdigit(ch)) {
					if (ch == 'j' || ch == KEY_DOWN) {
						view_select(view_sel + atoi(cmd));
					} else if (ch == 'k' || ch == KEY_UP) {
						view_select(view_sel - atoi(cmd));
					} else if (ch == 'G') {
						view_select(atoi(cmd) - 1);
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
run_feeds_menu(void)
{
	view_min = 0;
	view_max = LINES - 1;
	clear();
	refresh();
	show_sets();

	int dest;
	bool status_cond;
	while ((dest = menu_feeds()) != MENU_QUIT) {
		dest = run_items_menu(set_list[view_sel].data);
		if (dest == MENU_FEEDS) {
			clear();
			refresh();
			show_sets();
			status_cond = is_feed_unread(set_list[view_sel].data);
			if (status_cond != set_list[view_sel].is_unread) {
				set_list[view_sel].is_unread = status_cond;
				set_expose(view_sel);
			}
			status_cond = is_feed_marked(set_list[view_sel].data);
			if (status_cond != set_list[view_sel].is_marked) {
				set_list[view_sel].is_marked = status_cond;
				set_expose(view_sel);
			}
		} else if (dest == MENU_ITEMS_EMPTY) {
			status_write("[empty] %s", set_image(&set_list[view_sel]));
		} else if (dest == MENU_QUIT) {
			break;
		}
	}

	return 0;
}
