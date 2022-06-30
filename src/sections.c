#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct feed_section {
	struct string *name;
	struct feed_line **feeds; // Array of pointers to related feed_line structs.
	size_t feeds_count; // Length of feeds array.
	size_t unread_count;
};

static struct feed_section *sections = NULL;
static size_t sections_count = 0;

static struct menu_list_settings sections_menu;

static struct format_arg fmt_args[] = {
	{L'n',  L"d", {.i = 0}},
	{L'u',  L"d", {.i = 0}},
	{L't',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0}}, // terminator
};

static const wchar_t *
write_section_entry(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = sections[index].unread_count;
	fmt_args[2].value.s = sections[index].name->ptr;
	return do_format(get_cfg_wstring(CFG_MENU_SECTION_ENTRY_FORMAT), fmt_args);
}

static int
paint_section_entry(size_t index)
{
	if (sections[index].unread_count > 0) {
		return CFG_COLOR_LIST_SECTION_UNREAD_FG;
	} else {
		return CFG_COLOR_LIST_SECTION_FG;
	}
}

static bool
create_new_section(const struct string *section_name)
{
	size_t section_index = sections_count++;
	struct feed_section *temp = realloc(sections, sizeof(struct feed_section) * sections_count);
	if (temp == NULL) {
		fputs("Not enough memory for another section structure!\n", stderr);
		return false;
	}
	sections = temp;
	sections[section_index].name = crtss(section_name);
	if (sections[section_index].name == NULL) {
		fputs("Not enough memory for section name string!\n", stderr);
		return false;
	}
	sections[section_index].feeds = NULL;
	sections[section_index].feeds_count = 0;
	sections[section_index].unread_count = 0;
	INFO("Created \"%s\" section.", section_name->ptr);
	return true;
}

bool
create_global_section(void)
{
	return create_new_section(get_cfg_string(CFG_GLOBAL_SECTION_NAME));
}

static inline struct feed_line *
find_feed_in_section(const struct string *link, const struct feed_section *section)
{
	for (size_t i = 0; i < section->feeds_count; ++i) {
		if ((link->len == section->feeds[i]->link->len) && (strcmp(link->ptr, section->feeds[i]->link->ptr) == 0)) {
			return section->feeds[i];
		}
	}
	return NULL;
}

static inline struct feed_line *
copy_feed_to_global_section(const struct feed_line *feed)
{
	struct feed_line *potential_duplicate = find_feed_in_section(feed->link, &sections[0]);
	if (potential_duplicate != NULL) {
		return potential_duplicate;
	}
	size_t feed_index = (sections[0].feeds_count)++;
	struct feed_line **temp = realloc(sections[0].feeds, sizeof(struct feed_line *) * sections[0].feeds_count);
	if (temp == NULL) {
		return NULL;
	}
	sections[0].feeds = temp;
	sections[0].feeds[feed_index] = malloc(sizeof(struct feed_line));
	if (sections[0].feeds[feed_index] == NULL) {
		return NULL;
	}
	if ((feed->name == NULL) || (feed->name->len == 0)) {
		sections[0].feeds[feed_index]->name = NULL;
	} else {
		sections[0].feeds[feed_index]->name = crtss(feed->name);
		if (sections[0].feeds[feed_index]->name == NULL) {
			return NULL;
		}
	}
	sections[0].feeds[feed_index]->link = crtss(feed->link);
	if (sections[0].feeds[feed_index]->link == NULL) {
		return NULL;
	}
	sections[0].feeds[feed_index]->unread_count = 0;
	return sections[0].feeds[feed_index];
}

static bool
attach_feed_to_section(struct feed_line *feed, struct feed_section *section)
{
	const struct feed_line *potential_duplicate = find_feed_in_section(feed->link, section);
	if (potential_duplicate == NULL) {
		size_t feed_index = (section->feeds_count)++;
		struct feed_line **temp = realloc(section->feeds, sizeof(struct feed_line *) * section->feeds_count);
		if (temp == NULL) {
			return false;
		}
		section->feeds = temp;
		section->feeds[feed_index] = feed;
	}
	return true;
}

bool
copy_feed_to_section(const struct feed_line *feed, const struct string *section_name)
{
	struct feed_line *attached_feed = copy_feed_to_global_section(feed);
	if (attached_feed == NULL) {
		fputs("Not enough memory for new feed in global section!\n", stderr);
		return false;
	}
	if (strcmp(section_name->ptr, sections[0].name->ptr) == 0) {
		// The section we add a feed to is global and we already added
		// a feed to the global section above. So exit innocently here.
		return true;
	}
	// Skip (i == 0) because first section is always the global one and
	// we already know that the section we add a feed to is not global.
	for (size_t i = 1; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			return attach_feed_to_section(attached_feed, &sections[i]);
		}
	}
	if (create_new_section(section_name) == false) {
		return false;
	}
	return attach_feed_to_section(attached_feed, &sections[sections_count - 1]);
}

void
obtain_feeds_of_global_section(struct feed_line ***feeds_ptr, size_t *feeds_count_ptr)
{
	*feeds_ptr = sections[0].feeds;
	*feeds_count_ptr = sections[0].feeds_count;
}

static inline void
refresh_unread_items_count_of_all_sections(void)
{
	for (size_t i = 0; i < sections_count; ++i) {
		sections[i].unread_count = 0;
		for (size_t j = 0; j < sections[i].feeds_count; ++j) {
			sections[i].unread_count += sections[i].feeds[j]->unread_count;
		}
	}
}

static void
reload_section(struct feed_line **feeds, size_t feeds_count)
{
	const char *failed_url;
	size_t errors = 0;

	for (size_t i = 0; i < feeds_count; ++i) {
		info_status("(%zu/%zu) Loading %s", i + 1, feeds_count, feeds[i]->link->ptr);
		if (update_and_refresh_feed(feeds[i]) == false) {
			failed_url = feeds[i]->link->ptr;
			fail_status("Failed to update %s", failed_url);
			++errors;
		}
	}

	if (errors == 0) {
		status_clean();
	} else if (errors == 1) {
		fail_status("Failed to update %s", failed_url);
	} else {
		fail_status("Failed to update %zu feeds (check out status history for more details)", errors);
	}

	refresh_unread_items_count_of_all_sections();
	expose_all_visible_entries_of_the_menu_list(&sections_menu);
}

static inline void
free_feed(struct feed_line *feed)
{
	if (feed != NULL) {
		free_string(feed->name);
		free_string(feed->link);
		free(feed);
	}
}

void
free_sections(void)
{
	size_t i;
	for (i = 0; i < sections[0].feeds_count; ++i) {
		free_feed(sections[0].feeds[i]);
	}
	for (i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		free(sections[i].feeds);
	}
	free(sections);
}

void
enter_sections_menu_loop(void)
{
	if (sections_count == 1) {
		INFO("There is no sections except for global one.");
		enter_feeds_menu_loop(sections[0].feeds, sections[0].feeds_count);
		return;
	}

	sections_menu.write_action = &write_section_entry;
	sections_menu.paint_action = &paint_section_entry;
	reset_menu_list_settings(&sections_menu, sections_count);

	refresh_unread_items_count_of_all_sections();
	redraw_menu_list(&sections_menu);

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		if (cmd == INPUT_SELECT_NEXT) {
			list_menu_select_next(&sections_menu);
		} else if (cmd == INPUT_SELECT_PREV) {
			list_menu_select_prev(&sections_menu);
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			list_menu_select_next_page(&sections_menu);
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			list_menu_select_prev_page(&sections_menu);
		} else if (cmd == INPUT_SELECT_FIRST) {
			list_menu_select_first(&sections_menu);
		} else if (cmd == INPUT_SELECT_LAST) {
			list_menu_select_last(&sections_menu);
		} else if (cmd == INPUT_MARK_READ) {
			// TODO
		} else if (cmd == INPUT_MARK_UNREAD) {
			// TODO
		} else if (cmd == INPUT_MARK_READ_ALL) {
			// TODO
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			// TODO
		} else if (cmd == INPUT_RELOAD) {
			reload_section(sections[sections_menu.view_sel].feeds, sections[sections_menu.view_sel].feeds_count);
		} else if (cmd == INPUT_RELOAD_ALL) {
			reload_section(sections[0].feeds, sections[0].feeds_count);
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_feeds_menu_loop(sections[sections_menu.view_sel].feeds, sections[sections_menu.view_sel].feeds_count);
			if (cmd == INPUT_QUIT_SOFT) {
				status_clean();
				refresh_unread_items_count_of_all_sections();
				redraw_menu_list(&sections_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_EXPLORE_MENU) {
			cmd = enter_items_menu_loop(sections[0].feeds, sections[0].feeds_count, CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT);
			if (cmd == INPUT_QUIT_SOFT) {
				refresh_unread_items_count_of_all_sections();
				redraw_menu_list(&sections_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_SOFT) {
				redraw_menu_list(&sections_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_RESIZE) {
			redraw_menu_list(&sections_menu);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}
}
