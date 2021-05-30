#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static struct feed_window *feed_list;
static int view_sel;
static int view_min;
static int view_max;
static int feed_count;

void
free_feed_list(void)
{
	if (feed_list == NULL) return;
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].feed != NULL) {
			if (feed_list[i].feed->name != NULL) free_string(&feed_list[i].feed->name);
			if (feed_list[i].feed->feed_url != NULL) free_string(&feed_list[i].feed->feed_url);
			if (feed_list[i].feed->site_url != NULL) free_string(&feed_list[i].feed->site_url);
			free(feed_list[i].feed);
		}
	}
	free(feed_list);
}

static bool
is_feed_marked(struct string *feed_url)
{
	if (feed_url == NULL) return false;
	bool is_marked = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND marked = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed is marked), say feed is marked
		if (sqlite3_step(res) == SQLITE_ROW) is_marked = true;
	} else {
		fprintf(stderr, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return is_marked;
}

static bool
is_feed_unread(struct string *feed_url)
{
	if (feed_url == NULL) return false;
	bool is_unread = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND unread = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed is unread), say feed is unread
		if (sqlite3_step(res) == SQLITE_ROW) is_unread = true;
	} else {
		fprintf(stderr, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return is_unread;
}

int
load_feed_list(void)
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

	int feed_index, error = 0, word_len;
	char c, word[1000];
	feed_list = NULL;
	feed_count = 0;
	view_sel = -1;

	while (1) {
		// get first character of the line
		c = fgetc(f);
		// skip white-space
		skip_chars(f, &c, " \t\n");
		// skip comment-line
		if (c == '#') { find_chars(f, &c, "\n"); continue; }
		if (c == EOF) break;
		feed_index = feed_count++;
		feed_list = realloc(feed_list, sizeof(struct feed_window) * feed_count);
		if (feed_list == NULL) {
			error = 1; fprintf(stderr, "memory allocation for feed list failed\n"); break;
		}
		feed_list[feed_index].is_marked = false;
		feed_list[feed_index].is_unread = false;
		feed_list[feed_index].feed = calloc(1, sizeof(struct feed_entry));
		if (feed_list[feed_index].feed == NULL) {
			error = 1; fprintf(stderr, "memory allocation for feed entry failed\n"); break;
		}
		word_len = 0;
		if (c == '@') { // create decoration
			while (1) {
				c = fgetc(f);
				if (c == '\r' || c == '\n' || c == EOF) { word[word_len] = '\0'; break; }
				word[word_len++] = c;
			}
			make_string(&feed_list[feed_index].feed->name, word, word_len);
			continue;
		} else { // create feed entry
			while (1) {
				word[word_len++] = c;
				c = fgetc(f);
				if (IS_WHITESPACE(c) || c == EOF) { word[word_len] = '\0'; break; }
			}
			make_string(&feed_list[feed_index].feed->feed_url, word, word_len);
			feed_list[feed_index].is_marked = is_feed_marked(feed_list[feed_index].feed->feed_url);
			feed_list[feed_index].is_unread = is_feed_unread(feed_list[feed_index].feed->feed_url);
		}
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
			make_string(&feed_list[feed_index].feed->name, word, word_len);
		}
		// move to the end of line
		find_chars(f, &c, "\n");
	}
	fclose(f);

	if (error != 0) {
		fprintf(stderr, "there is some trouble in loading feeds!\n");
		free_feed_list();
		return 1;
	}

	// put first non-decorative feed entry in selection
	for (feed_index = 0; feed_index < feed_count; ++feed_index) {
		if (feed_list[feed_index].feed->feed_url != NULL) {
			view_sel = feed_index;
			break;
		}
	}

	if (view_sel == -1) {
		fprintf(stderr, "none of feeds loaded!\n");
		free_feed_list();
		return 1;
	}

	return 0; // success
}

// return most sensible string for feed title
static char *
feed_image(struct feed_entry *feed)
{
	if (feed->name != NULL) {
		return feed->name->ptr;
	} else {
		if (feed->feed_url != NULL) {
			return feed->feed_url->ptr;
		} else {
			return "";
		}
	}
}

static void
feed_expose(int index)
{
	struct feed_window *win = &feed_list[index];
	if (config_menu_show_number == true) {
		if (win->feed->feed_url != NULL) mvwprintw(win->window, 0, 0, "%3d", index + 1);
		mvwprintw(win->window, 0, 5, win->is_marked ? "M" : " ");
		mvwprintw(win->window, 0, 6, win->is_unread ? "N" : " ");
		mvwprintw(win->window, 0, 9, "%s", feed_image(win->feed));
	} else {
		mvwprintw(win->window, 0, 2, win->is_marked ? "M" : " ");
		mvwprintw(win->window, 0, 3, win->is_unread ? "N" : " ");
		mvwprintw(win->window, 0, 6, "%s", feed_image(win->feed));
	}
	mvwchgat(win->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(win->window);
}

static void
show_feeds(void)
{
	for (int i = view_min, j = 0; i < feed_count && i < view_max; ++i, ++j) {
		feed_list[i].window = newwin(1, COLS, j, 0);
		feed_expose(i);
	}
}

void
hide_feeds(void)
{
	for (int i = view_min; i < feed_count && i < view_max; ++i) {
		if (feed_list[i].window != NULL) {
			delwin(feed_list[i].window);
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
	} else if (new_sel >= feed_count) {
		new_sel = feed_count - 1;
		if (new_sel < 0) return;
	}

	// skip decorations
	if (feed_list[new_sel].feed->feed_url == NULL) {
		if (new_sel > view_sel) {
			for (j = new_sel + 1; (j < feed_count) && (feed_list[j].feed->feed_url == NULL); ++j);
			if (j < feed_count) {
				new_sel = j;
			} else {
				for (j = view_sel; j < feed_count; ++j) {
					if (feed_list[j].feed->feed_url != NULL) new_sel = j;
				}
			}
		} else if (new_sel < view_sel) {
			for (j = new_sel - 1; (j > -1) && (feed_list[j].feed->feed_url == NULL); --j);
			if (j > -1) {
				new_sel = j;
			} else {
				for (j = view_sel; j > -1; --j) {
					if (feed_list[j].feed->feed_url != NULL) new_sel = j;
				}
			}
		}
	}

	if (feed_list[new_sel].feed->feed_url != NULL && new_sel != view_sel) {
		if (new_sel >= view_max) {
			hide_feeds();
			view_min += new_sel - view_sel;
			view_max += new_sel - view_sel;
			if (view_max > feed_count) {
				view_max = feed_count;
				view_min = view_max - LINES + 1;
			}
			view_sel = new_sel;
			show_feeds();
		} else if (new_sel < view_min) {
			hide_feeds();
			view_min -= view_sel - new_sel;
			view_max -= view_sel - new_sel;
			if (view_min < 0) {
				view_min = 0;
				view_max = LINES - 1;
			}
			view_sel = new_sel;
			show_feeds();
		} else {
			old_sel = view_sel;
			view_sel = new_sel;
			feed_expose(old_sel);
			feed_expose(view_sel);
		}
	}
}

static void
feed_reload(int index)
{
	struct feed_window *win = &feed_list[index];
	status_write("[loading] %s", feed_image(win->feed));
	struct string *buf = feed_download(win->feed->feed_url->ptr);
	if (buf == NULL) return;
	if (buf->ptr == NULL) { free(buf); return; }
	if (feed_process(buf, win->feed) == 0) {
		status_clean();
		bool unread_status = is_feed_unread(win->feed->feed_url);
		if (win->is_unread != unread_status) {
			win->is_unread = unread_status;
			feed_expose(index);
		}
	}
	free_string(&buf);
}

static void
feed_reload_all(void)
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
		if      (ch == 'j' || ch == KEY_DOWN)                                   { view_select(view_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)                                     { view_select(view_sel - 1); }
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_ITEMS; }
		else if (ch == config_key_soft_quit || ch == config_key_hard_quit)      { return MENU_QUIT; }
		else if (ch == config_key_download)                                     { feed_reload(view_sel); }
		else if (ch == config_key_download_all)                                 { feed_reload_all(); }
		else if (ch == 'g' && input_wgetch() == 'g')                            { view_select(0); }
		else if (ch == 'G')                                                     { view_select(feed_count - 1); }
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
	}
}

int
run_feeds_menu(void)
{
	view_min = 0;
	view_max = LINES - 1;
	clear();
	refresh();
	show_feeds();

	int dest;
	bool status_cond;
	while ((dest = menu_feeds()) != MENU_QUIT) {
		dest = run_items_menu(feed_list[view_sel].feed->feed_url);
		if (dest == MENU_FEEDS) {
			clear();
			refresh();
			show_feeds();
			status_cond = is_feed_unread(feed_list[view_sel].feed->feed_url);
			if (status_cond != feed_list[view_sel].is_unread) {
				feed_list[view_sel].is_unread = status_cond;
				feed_expose(view_sel);
			}
			status_cond = is_feed_marked(feed_list[view_sel].feed->feed_url);
			if (status_cond != feed_list[view_sel].is_marked) {
				feed_list[view_sel].is_marked = status_cond;
				feed_expose(view_sel);
			}
		} else if (dest == MENU_ITEMS_EMPTY) {
			status_write("[empty] %s", feed_image(feed_list[view_sel].feed));
		} else if (dest == MENU_QUIT) {
			break;
		}
	}

	return 0;
}
