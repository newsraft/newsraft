#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct feed_section {
	struct string *name;
	struct feed_entry **feeds; // Array of pointers to feeds belonging to this section.
	size_t feeds_count;
	size_t unread_count;
};

static struct feed_section *sections = NULL;
static size_t sections_count = 0;

static bool
is_section_valid(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return index < sections_count ? true : false;
}

static const struct format_arg *
get_section_args(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	static struct format_arg section_fmt[] = {
		{L'i',  L'd',  {.i = 0   }},
		{L'u',  L'd',  {.i = 0   }},
		{L't',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	section_fmt[0].value.i = index + 1;
	section_fmt[1].value.i = sections[index].unread_count;
	section_fmt[2].value.s = sections[index].name->ptr;
	return section_fmt;
}

static int
paint_section(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return sections[index].unread_count > 0 ? CFG_COLOR_LIST_SECTION_UNREAD : CFG_COLOR_LIST_SECTION;
}

static bool
is_section_unread(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return sections[index].unread_count > 0;
}

#ifdef TEST
struct feed_entry **
get_all_feeds(size_t *feeds_count)
{
	*feeds_count = sections[0].feeds_count;
	return sections[0].feeds;
}
#endif

void
mark_feeds_read(struct feed_entry **feeds, size_t feeds_count, bool status)
{
	if (db_change_unread_status_of_all_items_in_feeds(feeds, feeds_count, !status) == true) {
		for (size_t i = 0; i < feeds_count; ++i) {
			int64_t new_unread_count = get_unread_items_count_of_the_feed(feeds[i]->link);
			if (new_unread_count >= 0) {
				feeds[i]->unread_count = new_unread_count;
			}
		}
		refresh_unread_items_count_of_all_sections();
		expose_all_visible_entries_of_the_list_menu();
		tell_items_menu_to_regenerate();
	}
}

int64_t
make_sure_section_exists(const struct string *section_name)
{
	for (size_t i = 0; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
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
	INFO("Created section \"%s\".", sections[sections_count].name->ptr);
	sections_count += 1;
	return sections_count - 1;
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
	struct feed_entry *duplicate = find_feed_in_section(feed->link, &sections[0]);
	if (duplicate != NULL) {
		return duplicate;
	}
	INFO("Adding to global section: %s", feed->link->ptr);
	size_t feed_index = (sections[0].feeds_count)++;
	struct feed_entry **temp = realloc(sections[0].feeds, sizeof(struct feed_entry *) * sections[0].feeds_count);
	if (temp == NULL) {
		return NULL;
	}
	sections[0].feeds = temp;
	sections[0].feeds[feed_index] = calloc(1, sizeof(struct feed_entry));
	if (sections[0].feeds[feed_index] == NULL) {
		return NULL;
	}

	if (feed->name != NULL && feed->name->len > 0) {
		sections[0].feeds[feed_index]->name = crtss(feed->name);
	} else {
		sections[0].feeds[feed_index]->name = db_get_string_from_feed_table(feed->link, "title", 5);
		if (sections[0].feeds[feed_index]->name != NULL) {
			inlinefy_string(sections[0].feeds[feed_index]->name);
		} else {
			sections[0].feeds[feed_index]->name = crtss(feed->link);
		}
	}
	if (sections[0].feeds[feed_index]->name == NULL) {
		return NULL;
	}

	sections[0].feeds[feed_index]->link = crtss(feed->link);
	if (sections[0].feeds[feed_index]->link == NULL) {
		return NULL;
	}

	int64_t new_unread_count = get_unread_items_count_of_the_feed(sections[0].feeds[feed_index]->link);
	if (new_unread_count < 0) {
		fputs("Failed to get unread items count of the feed from database!\n", stderr);
		return NULL;
	}
	sections[0].feeds[feed_index]->unread_count  = new_unread_count;
	sections[0].feeds[feed_index]->download_date = db_get_date_from_feeds_table(feed->link, "download_date", 13);
	return sections[0].feeds[feed_index];
}

struct feed_entry *
copy_feed_to_section(const struct feed_entry *feed_data, int64_t section_index)
{
	// All feeds without exception are stored in the global section
	struct feed_entry *feed = copy_feed_to_global_section(feed_data);
	if (feed == NULL) {
		fputs("Not enough memory!\n", stderr);
		return NULL;
	}

	// User sections contain only pointers to feeds in the global section
	struct feed_section *section = sections + section_index;
	if (find_feed_in_section(feed->link, section) == NULL) {
		struct feed_entry **f = realloc(section->feeds, sizeof(struct feed_entry *) * (section->feeds_count + 1));
		if (f == NULL) {
			fputs("Not enough memory!\n", stderr);
			return NULL;
		}
		section->feeds = f;
		section->feeds[section->feeds_count] = feed;
		section->feeds_count += 1;
	}

	return feed;
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

bool
purge_abandoned_feeds(void)
{
	if (sections[0].feeds_count == 0) {
		return true;
	}
	struct string *query = crtas("DELETE FROM items WHERE feed_url NOT IN (?", 42);
	if (query == NULL) {
		return false;
	}
	for (size_t i = 1; i < sections[0].feeds_count; ++i) {
		if (catas(query, ",?", 2) == false) {
			free_string(query);
			return false;
		}
	}
	if (catcs(query, ')') == false) {
		free_string(query);
		return false;
	}
	sqlite3_stmt *res = db_prepare(query->ptr, query->len + 1);
	if (res == NULL) {
		free_string(query);
		return false;
	}
	for (size_t i = 0; i < sections[0].feeds_count; ++i) {
		db_bind_string(res, i + 1, sections[0].feeds[i]->link);
	}
	const bool status = sqlite3_step(res) == SQLITE_DONE ? true : false;
	sqlite3_finalize(res);
	free_string(query);
	return status;
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

void
process_auto_updating_feeds(void)
{
	time_t now = time(NULL);
	if (now > 0) {
		for (size_t i = 0; i < sections[0].feeds_count; ++i) {
			size_t download = sections[0].feeds[i]->download_date;
			size_t age = (size_t)now > download ? now - download : 0;
			size_t reload_period = get_cfg_uint(&sections[0].feeds[i]->cfg, CFG_RELOAD_PERIOD) * 60;
			if (reload_period > 0 && age > reload_period) {
				update_feeds(sections[0].feeds + i, 1);
			}
		}
	}
}

struct menu_state *
sections_menu_loop(struct menu_state *m)
{
	m->enumerator   = &is_section_valid;
	m->get_args     = &get_section_args;
	m->paint_action = &paint_section;
	m->unread_state = &is_section_unread;
	m->write_action = &list_menu_writer;
	m->entry_format = get_cfg_wstring(NULL, CFG_MENU_SECTION_ENTRY_FORMAT);
	if (!(m->flags & MENU_DISABLE_SETTINGS)) {
		if (get_cfg_bool(NULL, CFG_SECTIONS_MENU_PARAMOUNT_EXPLORE) && get_items_count_of_feeds(sections[0].feeds, sections[0].feeds_count)) {
			return setup_menu(&items_menu_loop, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE);
		} else if (sections_count == 1) {
			return setup_menu(&feeds_menu_loop, sections[0].feeds, sections[0].feeds_count, MENU_SWALLOW);
		}
	}
	refresh_unread_items_count_of_all_sections();
	start_menu();
	const struct wstring *macro;
	for (input_cmd_id cmd = get_input_cmd(NULL, &macro) ;; cmd = get_input_cmd(NULL, &macro)) {
		if (handle_list_menu_control(m, cmd, macro) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(sections[m->view_sel].feeds, sections[m->view_sel].feeds_count, true);  break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(sections[m->view_sel].feeds, sections[m->view_sel].feeds_count, false); break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(sections[0].feeds, sections[0].feeds_count, true);                      break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(sections[0].feeds, sections[0].feeds_count, false);                     break;
			case INPUT_RELOAD:          update_feeds(sections[m->view_sel].feeds, sections[m->view_sel].feeds_count);           break;
			case INPUT_RELOAD_ALL:      update_feeds(sections[0].feeds, sections[0].feeds_count);                               break;
			case INPUT_ENTER:
				return setup_menu(&feeds_menu_loop, sections[m->view_sel].feeds, sections[m->view_sel].feeds_count, MENU_NO_FLAGS);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE | MENU_USE_SEARCH);
			case INPUT_STATUS_HISTORY_MENU:
				return setup_menu(&status_pager_loop, NULL, 0, MENU_NO_FLAGS);
			case INPUT_QUIT_SOFT:
			case INPUT_QUIT_HARD:
				return NULL;
		}
	}
	return NULL;
}
