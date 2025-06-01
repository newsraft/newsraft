#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct feed_section {
	struct string *name;
	struct feed_entry **feeds; // Array of pointers to feeds belonging to this section.
	size_t feeds_count;
	size_t unread_count;
	bool has_errors;
};

static struct feed_section *sections = NULL;
static struct feed_section **sections_view = NULL;
static size_t sections_count = 0;
static sorting_method_t sections_sort = SORT_BY_INITIAL_ASC;

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
	section_fmt[1].value.i = sections_view[index]->unread_count;
	section_fmt[2].value.s = sections_view[index]->name->ptr;
	return section_fmt;
}

static struct config_color
paint_section(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	if (sections_view[index]->has_errors) {
		return get_cfg_color(NULL, CFG_COLOR_LIST_SECTION_FAILED);
	} else if (sections_view[index]->unread_count > 0) {
		return get_cfg_color(NULL, CFG_COLOR_LIST_SECTION_UNREAD);
	} else {
		return get_cfg_color(NULL, CFG_COLOR_LIST_SECTION);
	}
}

static bool
is_section_unread(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return sections_view[index]->unread_count > 0;
}

static bool
is_section_failed(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return sections_view[index]->has_errors;
}

struct feed_entry **
get_all_feeds(size_t *feeds_count)
{
	*feeds_count = sections[0].feeds_count;
	return sections[0].feeds;
}

void
mark_feeds_read(struct feed_entry **feeds, size_t feeds_count, bool status)
{
	if (db_change_unread_status_of_all_items_in_feeds(feeds, feeds_count, !status) == true) {
		for (size_t i = 0; i < feeds_count; ++i) {
			feeds[i]->unread_count = db_count_items(&feeds[i], 1, true);
		}
		refresh_sections_statistics_about_underlying_feeds();
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
	sections = newsraft_realloc(sections, sizeof(struct feed_section) * (sections_count + 1));
	sections_view = newsraft_realloc(sections_view, sizeof(struct feed_section *) * (sections_count + 1));
	memset(&sections[sections_count], 0, sizeof(struct feed_section));
	sections[sections_count].name = crtss(section_name);
	if (sections[sections_count].name == NULL) {
		write_error("Not enough memory for section name string!\n");
		return -1;
	}
	INFO("Created section \"%s\".", sections[sections_count].name->ptr);
	sections_count += 1;

	// Have to update it every time because primary array is realloc'ed just above.
	for (size_t i = 0; i < sections_count; ++i) {
		sections_view[i] = &sections[i];
	}

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
	sections[0].feeds = newsraft_realloc(sections[0].feeds, sizeof(struct feed_entry *) * sections[0].feeds_count);
	sections[0].feeds[feed_index] = newsraft_calloc(1, sizeof(struct feed_entry));

	if (STRING_IS_EMPTY(feed->name)) {
		sections[0].feeds[feed_index]->name = db_get_string_from_feed_table(feed->link, "title", 5);
	} else {
		sections[0].feeds[feed_index]->name = crtss(feed->name);
	}
	if (sections[0].feeds[feed_index]->name != NULL) {
		inlinefy_string(sections[0].feeds[feed_index]->name);
	}

	sections[0].feeds[feed_index]->link   = crtss(feed->link);
	sections[0].feeds[feed_index]->errors = crtes(1);

	sections[0].feeds[feed_index]->update_date = db_get_date_from_feeds_table(feed->link, "update_date", 11);
	return sections[0].feeds[feed_index];
}

struct feed_entry *
copy_feed_to_section(const struct feed_entry *feed_data, int64_t section_index)
{
	// All feeds without exception are stored in the global section
	struct feed_entry *feed = copy_feed_to_global_section(feed_data);
	if (feed == NULL) {
		write_error("Not enough memory!\n");
		return NULL;
	}

	// User sections contain only pointers to feeds in the global section
	struct feed_section *section = sections + section_index;
	if (find_feed_in_section(feed->link, section) == NULL) {
		section->feeds = newsraft_realloc(section->feeds, sizeof(struct feed_entry *) * (section->feeds_count + 1));
		section->feeds[section->feeds_count] = feed;
		section->feeds_count += 1;
	}

	return feed;
}

void
refresh_sections_statistics_about_underlying_feeds(void)
{
	pthread_mutex_lock(&interface_lock);
	for (size_t i = 0; i < sections_count; ++i) {
		bool has_errors = false;
		sections[i].unread_count = 0;
		for (size_t j = 0; j < sections[i].feeds_count; ++j) {
			if (sections[i].feeds[j]->errors->len > 0 && !get_cfg_bool(&sections[i].feeds[j]->cfg, CFG_SUPPRESS_ERRORS)) {
				has_errors = true;
			}
			sections[i].unread_count += sections[i].feeds[j]->unread_count;
		}
		sections[i].has_errors = has_errors;
	}
	pthread_mutex_unlock(&interface_lock);
}

bool
purge_abandoned_feeds(void)
{
	if (sections[0].feeds_count == 0) {
		return true;
	}
	char *query = newsraft_malloc(sizeof(char) * (42 + sections[0].feeds_count * 2 + 100));
	strcpy(query, "DELETE FROM items WHERE feed_url NOT IN (?");
	for (size_t i = 1; i < sections[0].feeds_count; ++i) {
		strcat(query, ",?");
	}
	strcat(query, ")");
	sqlite3_stmt *res = db_prepare(query, strlen(query) + 1, NULL);
	if (res == NULL) {
		free(query);
		return false;
	}
	for (size_t i = 0; i < sections[0].feeds_count; ++i) {
		db_bind_string(res, i + 1, sections[0].feeds[i]->link);
	}
	bool status = sqlite3_step(res) == SQLITE_DONE ? true : false;
	sqlite3_finalize(res);
	free(query);
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
			free_string(sections[0].feeds[i]->errors);
			free_config_context(sections[0].feeds[i]->cfg);
			if (sections[0].feeds[i]->binds != NULL) {
				free_binds(sections[0].feeds[i]->binds);
			}
			newsraft_free(sections[0].feeds[i]);
		}
	}
	for (i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		newsraft_free(sections[i].feeds);
	}
	newsraft_free(sections);
}

bool
start_updating_all_feeds_and_wait_finish(void)
{
	queue_updates(sections[0].feeds, sections[0].feeds_count);
	queue_wait_finish();
	return true;
}

bool
print_unread_items_count(void)
{
	int64_t count = db_count_items(sections[0].feeds, sections[0].feeds_count, true);
	fprintf(stdout, "%" PRId64 "\n", count);
	fflush(stdout);
	return true;
}

void
process_auto_updating_feeds(void)
{
	time_t now = time(NULL);
	if (now > 0) {
		for (size_t i = 0; i < sections[0].feeds_count; ++i) {
			size_t last_try = sections[0].feeds[i]->update_date;
			size_t age = (size_t)now > last_try ? now - last_try : 0;
			size_t reload_period = get_cfg_uint(&sections[0].feeds[i]->cfg, CFG_RELOAD_PERIOD) * 60;
			if (reload_period > 0 && age > reload_period) {
				queue_updates(sections[0].feeds + i, 1);
			}
		}
	}
}

static int
compare_sections_initial(const void *data1, const void *data2)
{
	struct feed_section **section1 = (struct feed_section **)data1;
	struct feed_section **section2 = (struct feed_section **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < sections_count; ++i) {
		if (*section1 == &sections[i]) index1 = i;
		if (*section2 == &sections[i]) index2 = i;
	}
	if (index1 > index2) return sections_sort & 1 ? -1 : 1;
	if (index1 < index2) return sections_sort & 1 ? 1 : -1;
	return 0;
}

static int
compare_sections_unread(const void *data1, const void *data2)
{
	struct feed_section *section1 = *(struct feed_section **)data1;
	struct feed_section *section2 = *(struct feed_section **)data2;
	if (section1->unread_count > section2->unread_count) return sections_sort & 1 ? -1 : 1;
	if (section1->unread_count < section2->unread_count) return sections_sort & 1 ? 1 : -1;
	return compare_sections_initial(data1, data2) * (sections_sort & 1 ? -1 : 1);
}

static int
compare_sections_alphabet(const void *data1, const void *data2)
{
	struct feed_section *section1 = *(struct feed_section **)data1;
	struct feed_section *section2 = *(struct feed_section **)data2;
	return strcmp(section1->name->ptr, section2->name->ptr) * (sections_sort & 1 ? -1 : 1);
}

static inline void
sort_sections(sorting_method_t method, bool we_are_already_in_sections_menu)
{
	pthread_mutex_lock(&interface_lock);
	sections_sort = method;
	switch (sections_sort & ~1) {
		case SORT_BY_UNREAD_ASC:   qsort(sections_view, sections_count, sizeof(struct feed_section *), &compare_sections_unread);   break;
		case SORT_BY_INITIAL_ASC:  qsort(sections_view, sections_count, sizeof(struct feed_section *), &compare_sections_initial);  break;
		case SORT_BY_ALPHABET_ASC: qsort(sections_view, sections_count, sizeof(struct feed_section *), &compare_sections_alphabet); break;
	}
	pthread_mutex_unlock(&interface_lock);
	if (we_are_already_in_sections_menu == true) {
		expose_all_visible_entries_of_the_list_menu();
		info_status(get_sorting_message(sections_sort), "sections");
	}
}

struct menu_state *
sections_menu_loop(struct menu_state *m)
{
	m->enumerator   = &is_section_valid;
	m->get_args     = &get_section_args;
	m->paint_action = &paint_section;
	m->unread_state = &is_section_unread;
	m->failed_state = &is_section_failed;
	m->write_action = &list_menu_writer;
	m->entry_format = get_cfg_wstring(NULL, CFG_MENU_SECTION_ENTRY_FORMAT);
	if (!(m->flags & MENU_DISABLE_SETTINGS)) {
		// Don't set the menu names here because it's redundant!
		if (get_cfg_bool(NULL, CFG_SECTIONS_MENU_PARAMOUNT_EXPLORE) && db_count_items(sections[0].feeds, sections[0].feeds_count, false)) {
			return setup_menu(&items_menu_loop, NULL, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE, NULL);
		} else if (sections_count == 1) {
			return setup_menu(&feeds_menu_loop, NULL, sections[0].feeds, sections[0].feeds_count, MENU_SWALLOW, NULL);
		}
	}
	refresh_sections_statistics_about_underlying_feeds();
	if (m->is_initialized == false) {
		sort_sections(get_sorting_id(get_cfg_string(NULL, CFG_MENU_SECTION_SORTING)->ptr), false);
	}
	start_menu();
	const struct wstring *arg;
	while (true) {
		input_id cmd = get_input(NULL, NULL, &arg);
		if (handle_list_menu_control(m, cmd, arg) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count, true);  break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count, false); break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(sections[0].feeds, sections[0].feeds_count, true);                                  break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(sections[0].feeds, sections[0].feeds_count, false);                                 break;
			case INPUT_RELOAD:          queue_updates(sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count);          break;
			case INPUT_RELOAD_ALL:      queue_updates(sections[0].feeds, sections[0].feeds_count);                                          break;
			case INPUT_ENTER:
				return setup_menu(&feeds_menu_loop, sections_view[m->view_sel]->name, sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count, MENU_NORMAL, NULL);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, NULL, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE, NULL);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, NULL, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE, NULL);
			case INPUT_VIEW_ERRORS:
				return setup_menu(&errors_pager_loop, NULL, sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count, MENU_NORMAL, NULL);
			case INPUT_SORT_BY_UNREAD:
				sort_sections(sections_sort == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC, true);
				break;
			case INPUT_SORT_BY_INITIAL:
				sort_sections(sections_sort == SORT_BY_INITIAL_ASC ? SORT_BY_INITIAL_DESC : SORT_BY_INITIAL_ASC, true);
				break;
			case INPUT_SORT_BY_ALPHABET:
				sort_sections(sections_sort == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC, true);
				break;
			case INPUT_DATABASE_COMMAND:
				db_perform_user_edit(arg, sections_view[m->view_sel]->feeds, sections_view[m->view_sel]->feeds_count, NULL);
				break;
			case INPUT_FIND_COMMAND:
				return setup_menu(&items_menu_loop, NULL, sections[0].feeds, sections[0].feeds_count, MENU_IS_EXPLORE, arg);
			case INPUT_QUIT_SOFT:
			case INPUT_QUIT_HARD:
				return NULL;
		}
	}
	return NULL;
}
