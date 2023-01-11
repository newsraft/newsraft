#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct feed_section {
	struct string *name;
	struct feed_entry **feeds; // Array of pointers to feeds belonging to this section.
	size_t feeds_count;
	size_t unread_count;
	int64_t update_period;
};

static struct feed_section *sections = NULL;
static size_t sections_count = 0;

static pthread_t auto_updater_routine_thread;
static volatile bool stop_auto_updater_routine = false;
static bool at_least_one_feed_has_positive_update_period = false;

static struct format_arg fmt_args[] = {
	{L'n',  L"d", {.i = 0   }},
	{L'u',  L"d", {.i = 0   }},
	{L't',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0   }}, // terminator
};

const struct format_arg *
prepare_section_entry_args(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = sections[index].unread_count;
	fmt_args[2].value.s = sections[index].name->ptr;
	return fmt_args;
}

int
paint_section_entry(size_t index)
{
	return sections[index].unread_count > 0 ? CFG_COLOR_LIST_SECTION_UNREAD_FG : CFG_COLOR_LIST_SECTION_FG;
}

bool
unread_section_condition(size_t index)
{
	return sections[index].unread_count > 0;
}

void
mark_selected_section_read(size_t view_sel)
{
	if (db_mark_all_items_in_feeds_as_read(sections[view_sel].feeds, sections[view_sel].feeds_count) == true) {
		for (size_t i = 0; i < sections[view_sel].feeds_count; ++i) {
			int64_t new_unread_count = get_unread_items_count_of_the_feed(sections[view_sel].feeds[i]->link);
			if (new_unread_count >= 0) {
				sections[view_sel].feeds[i]->unread_count = new_unread_count;
			}
		}
		refresh_unread_items_count_of_all_sections();
		expose_all_visible_entries_of_the_list_menu();
	}
}

void
mark_selected_section_unread(size_t view_sel)
{
	if (db_mark_all_items_in_feeds_as_unread(sections[view_sel].feeds, sections[view_sel].feeds_count) == true) {
		for (size_t i = 0; i < sections[view_sel].feeds_count; ++i) {
			int64_t new_unread_count = get_unread_items_count_of_the_feed(sections[view_sel].feeds[i]->link);
			if (new_unread_count >= 0) {
				sections[view_sel].feeds[i]->unread_count = new_unread_count;
			}
		}
		refresh_unread_items_count_of_all_sections();
		expose_all_visible_entries_of_the_list_menu();
	}
}

int64_t
make_sure_section_exists(const struct string *section_name, int64_t update_period)
{
	for (size_t i = 0; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			sections[i].update_period = update_period;
			return i;
		}
	}
	struct feed_section *tmp = realloc(sections, sizeof(struct feed_section) * (sections_count + 1));
	if (tmp == NULL) {
		fputs("Not enough memory for another section structure!\n", stderr);
		return -1;
	}
	sections = tmp;
	sections[sections_count].name = crtss(section_name);
	if (sections[sections_count].name == NULL) {
		fputs("Not enough memory for section name string!\n", stderr);
		return -1;
	}
	sections[sections_count].feeds = NULL;
	sections[sections_count].feeds_count = 0;
	sections[sections_count].unread_count = 0;
	sections[sections_count].update_period = update_period;
	INFO("Created section \"%s\" with update period %" PRId64 ".", sections[sections_count].name->ptr, update_period);
	sections_count += 1;
	return sections_count - 1;
}

bool
create_global_section(void)
{
	return make_sure_section_exists(get_cfg_string(CFG_GLOBAL_SECTION_NAME), -1) == 0 ? true : false;
}

static inline struct feed_entry *
find_feed_in_section(const struct string *link, const struct feed_section *section)
{
	for (size_t i = 0; i < section->feeds_count; ++i) {
		if ((link->len == section->feeds[i]->link->len) && (strcmp(link->ptr, section->feeds[i]->link->ptr) == 0)) {
			return section->feeds[i];
		}
	}
	return NULL;
}

static inline struct feed_entry *
copy_feed_to_global_section(const struct feed_entry *feed)
{
	struct feed_entry *potential_duplicate = find_feed_in_section(feed->link, &sections[0]);
	if (potential_duplicate != NULL) {
		return potential_duplicate;
	}
	INFO("Copying feed %s with update time period %" PRId64 " to global section.", feed->link->ptr, feed->update_period);
	size_t feed_index = (sections[0].feeds_count)++;
	struct feed_entry **temp = realloc(sections[0].feeds, sizeof(struct feed_entry *) * sections[0].feeds_count);
	if (temp == NULL) {
		return NULL;
	}
	sections[0].feeds = temp;
	sections[0].feeds[feed_index] = malloc(sizeof(struct feed_entry));
	if (sections[0].feeds[feed_index] == NULL) {
		return NULL;
	}
	if ((feed->name == NULL) || (feed->name->len == 0)) {
		sections[0].feeds[feed_index]->name = db_get_string_from_feed_table(feed->link, "title", 5);
		if (sections[0].feeds[feed_index]->name != NULL) {
			inlinefy_string(sections[0].feeds[feed_index]->name);
		}
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
	int64_t new_unread_count = get_unread_items_count_of_the_feed(sections[0].feeds[feed_index]->link);
	if (new_unread_count < 0) {
		return NULL;
	}
	sections[0].feeds[feed_index]->unread_count = new_unread_count;
	sections[0].feeds[feed_index]->update_period = feed->update_period;
	sections[0].feeds[feed_index]->update_iterator = 0;
	if (feed->update_period > 0) {
		at_least_one_feed_has_positive_update_period = true;
	}
	return sections[0].feeds[feed_index];
}

static bool
attach_feed_to_section(struct feed_entry *feed, struct feed_section *section)
{
	const struct feed_entry *potential_duplicate = find_feed_in_section(feed->link, section);
	if (potential_duplicate == NULL) {
		size_t feed_index = (section->feeds_count)++;
		struct feed_entry **temp = realloc(section->feeds, sizeof(struct feed_entry *) * section->feeds_count);
		if (temp == NULL) {
			return false;
		}
		section->feeds = temp;
		section->feeds[feed_index] = feed;
	}
	return true;
}

bool
copy_feed_to_section(const struct feed_entry *feed, int64_t section_index)
{
	struct feed_entry *attached_feed = copy_feed_to_global_section(feed);
	if (attached_feed == NULL) {
		fputs("Not enough memory for new feed in global section!\n", stderr);
		return false;
	}
	return attach_feed_to_section(attached_feed, &sections[section_index]);
}

void
refresh_unread_items_count_of_all_sections(void)
{
	for (size_t i = 0; i < sections_count; ++i) {
		sections[i].unread_count = 0;
		for (size_t j = 0; j < sections[i].feeds_count; ++j) {
			sections[i].unread_count += sections[i].feeds[j]->unread_count;
		}
	}
}

void
free_sections(void)
{
	size_t i;
	for (i = 0; i < sections[0].feeds_count; ++i) {
		if (sections[0].feeds[i] != NULL) {
			free_string(sections[0].feeds[i]->name);
			free_string(sections[0].feeds[i]->link);
			free(sections[0].feeds[i]);
		}
	}
	for (i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		free(sections[i].feeds);
	}
	free(sections);
}

static void *
auto_updater_routine(void *dummy)
{
	(void)dummy;
	size_t i;
	const struct timespec auto_updater_interval = {0, 500000000L}; // 0.5 seconds
	while (stop_auto_updater_routine == false) {
		for (i = 0; i < sections[0].feeds_count; ++i) {
			if (sections[0].feeds[i]->update_period > 0) {
				if ((sections[0].feeds[i]->update_iterator % sections[0].feeds[i]->update_period) == 0) {
					update_feeds(sections[0].feeds + i, 1);
				}
				sections[0].feeds[i]->update_iterator += 1;
			} else if ((sections[0].feeds[i]->update_period < 0) && (sections[0].update_period > 0)) {
				if ((sections[0].feeds[i]->update_iterator % sections[0].update_period) == 0) {
					update_feeds(sections[0].feeds + i, 1);
				}
				sections[0].feeds[i]->update_iterator += 1;
			}
		}
		// Sleep for a total of 1 minute while checking if they want us to stop.
		for (i = 0; (i < 120) && (stop_auto_updater_routine == false); ++i) {
			nanosleep(&auto_updater_interval, NULL);
		}
	}
	return NULL;
}

bool
start_auto_updater_if_necessary(void)
{
	if ((at_least_one_feed_has_positive_update_period == true) || (sections[0].update_period > 0)) {
		if (pthread_create(&auto_updater_routine_thread, NULL, &auto_updater_routine, NULL) != 0) {
			fputs("Failed to create auto updater thread!\n", stderr);
			return false;
		}
	}
	return true;
}

void
finish_auto_updater_if_necessary(void)
{
	if ((at_least_one_feed_has_positive_update_period == true) || (sections[0].update_period > 0)) {
		stop_auto_updater_routine = true;
		pthread_join(auto_updater_routine_thread, NULL);
	}
}

void
enter_sections_menu_loop(void)
{
	if (sections_count == 1) {
		enter_feeds_menu_loop(sections[0].feeds, sections[0].feeds_count);
		return;
	}

	const size_t *view_sel = enter_list_menu(SECTIONS_MENU, sections_count, CFG_MENU_SECTION_ENTRY_FORMAT);

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		if (handle_list_menu_navigation(cmd) == true) {
			// Rest a little.
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_selected_section_read(0);
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_selected_section_unread(0);
		} else if (cmd == INPUT_RELOAD) {
			update_feeds(sections[*view_sel].feeds, sections[*view_sel].feeds_count);
		} else if (cmd == INPUT_RELOAD_ALL) {
			update_feeds(sections[0].feeds, sections[0].feeds_count);
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_feeds_menu_loop(sections[*view_sel].feeds, sections[*view_sel].feeds_count);
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_EXPLORE_MENU) {
			cmd = enter_items_menu_loop(sections[0].feeds, sections[0].feeds_count, CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT);
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}
}
