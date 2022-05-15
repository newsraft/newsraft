#include <stdlib.h>
#include "feedeater.h"

static struct feed_line **feeds = NULL;
static size_t feeds_count = 0;

static struct menu_list_settings feeds_menu;

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"d", {.i = 0}},
	{L't', L"s", {.s = NULL}},
};

// On success returns 0.
// On failure returns non-zero.
static inline bool
parse_feeds_file(const char *path)
{
	struct string *word = crtes();
	if (word == NULL) {
		return false;
	}
	const struct string *global_section_name = get_cfg_string(CFG_GLOBAL_SECTION_NAME);
	struct string *section_name = crtss(global_section_name);
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
	const char *feeds_file_path = get_feeds_path();
	if (feeds_file_path == NULL) {
		// Error message is written by get_feeds_path().
		return false;
	}
	if (create_global_section() == false) {
		fprintf(stderr, "Not enough memory for global section structure!\n");
		return false;
	}
	if (parse_feeds_file(feeds_file_path) == false) {
		fprintf(stderr, "Failed to load feeds from file!\n");
		return false;
	}

	// Display feeds of global section (that is all feeds) by default.
	obtain_feeds_of_global_section(&feeds, &feeds_count);

	if (feeds_count == 0) {
		fprintf(stderr, "Not a single feed was loaded!\n");
		return false;
	}

	for (size_t i = 0; i < feeds_count; ++i) {
		feeds[i]->unread_count = get_unread_items_count_of_the_feed(feeds[i]->link);
		if (feeds[i]->unread_count < 0) {
			fprintf(stderr, "Failed to get unread items count of the \"%s\" feed!\n", feeds[i]->link->ptr);
			return false;
		}
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

static const wchar_t *
paint_feed_entry(size_t index)
{
	const struct wstring *format = get_cfg_wstring(CFG_MENU_FEED_ENTRY_FORMAT);
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = feeds[index]->unread_count;
	fmt_args[2].value.s = feed_image(feeds[index]);
	return do_format(format, fmt_args, COUNTOF(fmt_args));
}

static void
update_unread_items_count(size_t index, bool redraw)
{
	int64_t new_unread_count = get_unread_items_count_of_the_feed(feeds[index]->link);
	if (new_unread_count < 0) {
		return;
	}
	if (feeds[index]->unread_count != new_unread_count) {
		feeds[index]->unread_count = new_unread_count;
		if ((redraw == true) && (index >= feeds_menu.view_min) && (index <= feeds_menu.view_max)) {
			expose_entry_of_the_menu_list(&feeds_menu, index);
		}
	}
}

static void
reload_current_feed(void)
{
	status_write("Loading %s", feeds[feeds_menu.view_sel]->link->ptr);

	if (update_feed(feeds[feeds_menu.view_sel]->link) == false) {
		status_write("Failed to load %s", feeds[feeds_menu.view_sel]->link->ptr);
		return;
	}

	update_unread_items_count(feeds_menu.view_sel, true);
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

static inline void
reset_menu_list_settings(void)
{
	feeds_menu.entries_count = feeds_count;
	feeds_menu.view_sel = 0;
	feeds_menu.view_min = 0;
	feeds_menu.view_max = list_menu_height - 1;
}

void
enter_feeds_menu_loop(void)
{
	feeds_menu.paint_action = &paint_feed_entry;
	reset_menu_list_settings();

	redraw_menu_list(&feeds_menu);

	input_cmd_id cmd;
	while (true) {
		cmd = get_input_command();
		if (cmd == INPUT_SELECT_NEXT) {
			list_menu_select_next(&feeds_menu);
		} else if (cmd == INPUT_SELECT_PREV) {
			list_menu_select_prev(&feeds_menu);
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			list_menu_select_next_page(&feeds_menu);
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			list_menu_select_prev_page(&feeds_menu);
		} else if (cmd == INPUT_SELECT_FIRST) {
			list_menu_select_first(&feeds_menu);
		} else if (cmd == INPUT_SELECT_LAST) {
			list_menu_select_last(&feeds_menu);
		} else if (cmd == INPUT_RELOAD) {
			reload_current_feed();
		} else if (cmd == INPUT_RELOAD_ALL) {
			reload_all_feeds();
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_items_menu_loop(feeds[feeds_menu.view_sel]->link);
			if (cmd == INPUT_QUIT_SOFT) {
				status_clean();
				update_unread_items_count(feeds_menu.view_sel, false);
				redraw_menu_list(&feeds_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_SECTIONS_MENU) {
			cmd = enter_sections_menu_loop(&feeds, &feeds_count);
			if (cmd == INPUT_QUIT_HARD) {
				break;
			} else if (cmd != INPUTS_COUNT) {
				status_clean();
				if (cmd == INPUT_ENTER) {
					reset_menu_list_settings();
				}
				redraw_menu_list(&feeds_menu);
			}
		} else if (cmd == INPUT_RESIZE) {
			redraw_menu_list(&feeds_menu);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}
}
