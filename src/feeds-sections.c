#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct feed_section {
	struct string *name;
	struct feed_line **feeds; // Array of pointers to related feed_line structs.
	size_t feeds_count; // Length of feeds array.
	size_t unread_count;
	WINDOW *window;
};

static struct feed_section *sections = NULL;
static size_t sections_count = 0;

static size_t view_sel; // index of selected section
static size_t view_min; // index of first visible section
static size_t view_max; // index of last visible section

static bool is_the_sections_menu_being_opened_for_the_first_time = true;

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"d", {.i = 0}},
	{L't', L"s", {.s = NULL}},
};

static inline void
free_feed(struct feed_line *feed)
{
	if (feed == NULL) {
		return;
	}
	free_string(feed->name);
	free_string(feed->link);
	free(feed);
}

static bool
create_new_section(const struct string *section_name)
{
	size_t section_index = sections_count++;
	struct feed_section *temp = realloc(sections, sizeof(struct feed_section) * sections_count);
	if (temp == NULL) {
		fprintf(stderr, "Not enough memory for another section structure!\n");
		return false;
	}
	sections = temp;
	sections[section_index].name = crtss(section_name);
	if (sections[section_index].name == NULL) {
		fprintf(stderr, "Not enough memory for name string of the section!\n");
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
	return create_new_section(cfg.global_section_name);
}

// On success returns a pointer to the attached feed.
// On failure returns NULL.
static struct feed_line *
attach_feed_to_section(struct feed_line *feed, struct feed_section *section, bool do_free)
{
	INFO("Adding feed \"%s\" to section \"%s\".", feed->link->ptr, section->name->ptr);
	// We have to make sure that this feed is unique.
	for (size_t i = 0; i < section->feeds_count; ++i) {
		if (strcmp(section->feeds[i]->link->ptr, feed->link->ptr) == 0) {
			INFO("This feed is already in this section.");
			if (do_free == true) {
				free_feed(feed);
			}
			return section->feeds[i];
		}
	}
	size_t feed_index = (section->feeds_count)++;
	struct feed_line **temp = realloc(section->feeds, sizeof(struct feed_line *) * section->feeds_count);
	if (temp == NULL) {
		fprintf(stderr, "Not enough memory for new feed in section!\n");
		return NULL;
	}
	section->feeds = temp;
	section->feeds[feed_index] = feed;
	return section->feeds[feed_index];
}

static inline struct feed_line *
add_feed_to_global_section(struct feed_line *feed)
{
	return attach_feed_to_section(feed, &sections[0], true);
}

static inline struct feed_line *
add_feed_to_regular_section(struct feed_line *feed, struct feed_section *section)
{
	return attach_feed_to_section(feed, section, false);
}

bool
add_feed_to_section(struct feed_line *feed, const struct string *section_name)
{
	if (feed->link == NULL) {
		fprintf(stderr, "Encountered a NULL feed link while adding a new feed to the section!\n");
		return false;
	}
	struct feed_line *attached_feed = add_feed_to_global_section(feed);
	if (attached_feed == NULL) {
		return false;
	}
	if (strcmp(section_name->ptr, cfg.global_section_name->ptr) == 0) {
		// The section we add a feed to is global and we already added
		// a feed to the global section above. So exit innocently here.
		return true; // Not an error.
	}
	// Skip (i == 0) because first section is always the global one and
	// we already know that the section we add a feed to is not global.
	for (size_t i = 1; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			if (add_feed_to_regular_section(attached_feed, &sections[i]) == NULL) {
				return false;
			}
			return true;
		}
	}
	if (create_new_section(section_name) == false) {
		return false;
	}
	if (add_feed_to_regular_section(attached_feed, &sections[sections_count - 1]) == NULL) {
		return false;
	}
	return true;
}

void
obtain_feeds_of_global_section(struct feed_line ***feeds_ptr, size_t *feeds_count_ptr)
{
	*feeds_ptr = sections[0].feeds;
	*feeds_count_ptr = sections[0].feeds_count;
}

void
free_sections(void)
{
	for (size_t i = 0; i < sections[0].feeds_count; ++i) {
		free_feed(sections[0].feeds[i]);
	}
	for (size_t i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		free(sections[i].feeds);
	}
	free(sections);
}

static void
expose_section(size_t index)
{
	werase(sections[index].window);
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = sections[index].unread_count;
	fmt_args[2].value.s = sections[index].name->ptr;
	mvwaddnwstr(sections[index].window, 0, 0, do_format(cfg.menu_section_entry_format, fmt_args, COUNTOF(fmt_args)), list_menu_width);
	mvwchgat(sections[index].window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(sections[index].window);
}

static void
show_sections(void)
{
	for (size_t i = view_min, j = 0; i < sections_count && i <= view_max; ++i, ++j) {
		sections[i].window = get_list_entry_by_index(j);
		expose_section(i);
	}
}

static void
view_select(size_t i)
{
	size_t new_sel = i;

	if (new_sel >= sections_count) {
		new_sel = sections_count - 1;
	}

	if (new_sel == view_sel) {
		return;
	}

	if (new_sel > view_max) {
		view_min = new_sel - (list_menu_height - 1);
		view_max = new_sel;
		view_sel = new_sel;
		show_sections();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + (list_menu_height - 1);
		view_sel = new_sel;
		show_sections();
	} else {
		mvwchgat(sections[view_sel].window, 0, 0, -1, A_NORMAL, 0, NULL);
		wrefresh(sections[view_sel].window);
		view_sel = new_sel;
		mvwchgat(sections[view_sel].window, 0, 0, -1, A_REVERSE, 0, NULL);
		wrefresh(sections[view_sel].window);
	}
}

static void
redraw_sections_windows(void)
{
	clear();
	refresh();
	status_update();
	view_max = view_min + (list_menu_height - 1);
	if (view_max < view_sel) {
		view_max = view_sel;
		view_min = view_max - (list_menu_height - 1);
	}
	show_sections();
}

input_cmd_id
enter_sections_menu_loop(struct feed_line ***feeds_ptr, size_t *feeds_count_ptr)
{
	if (sections_count == 1) {
		status_write("There is no sections except for global one!");
		return INPUTS_COUNT;
	}

	if (is_the_sections_menu_being_opened_for_the_first_time == true) {
		view_sel = 0;
		view_min = 0;
		view_max = list_menu_height - 1;
		is_the_sections_menu_being_opened_for_the_first_time = false;
	}

	status_clean();
	redraw_sections_windows();

	input_cmd_id cmd;
	while (true) {
		cmd = get_input_command();
		if (cmd == INPUT_SELECT_NEXT) {
			view_select(view_sel + 1);
		} else if (cmd == INPUT_SELECT_PREV) {
			view_select(view_sel == 0 ? (0) : (view_sel - 1));
		} else if (cmd == INPUT_SELECT_FIRST) {
			view_select(0);
		} else if (cmd == INPUT_SELECT_LAST) {
			view_select(sections_count - 1);
		} else if (cmd == INPUT_MARK_READ) {
			// TODO
		} else if (cmd == INPUT_MARK_UNREAD) {
			// TODO
		} else if (cmd == INPUT_MARK_READ_ALL) {
			// TODO
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			// TODO
		} else if (cmd == INPUT_ENTER) {
			*feeds_ptr = sections[view_sel].feeds;
			*feeds_count_ptr = sections[view_sel].feeds_count;
			break;
		} else if (cmd == INPUT_RESIZE) {
			redraw_sections_windows();
		} else if ((cmd == INPUT_SECTIONS_MENU) || (cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}

	return cmd;
}
