#ifndef ACTIONS_H
#define ACTIONS_H

struct action_descriptor {
	const char *names[10];
};

#define ACTION(NAME, ...) NAME,
enum {

#endif // ACTIONS_H

#ifdef ACTIONS_IMPL
#define ACTION(NAME, ...) [NAME] = {__VA_ARGS__},
static struct action_descriptor g_actions[] = {
#endif // ACTIONS_IMPL

ACTION(INPUT_INVALID)
ACTION(INPUT_SELECT_NEXT,              "select-next")
ACTION(INPUT_SELECT_PREV,              "select-prev")
ACTION(INPUT_SELECT_NEXT_PAGE,         "select-next-page")
ACTION(INPUT_SELECT_NEXT_PAGE_HALF,    "select-next-page-half")
ACTION(INPUT_SELECT_PREV_PAGE,         "select-prev-page")
ACTION(INPUT_SELECT_PREV_PAGE_HALF,    "select-prev-page-half")
ACTION(INPUT_SELECT_FIRST,             "select-first")
ACTION(INPUT_SELECT_LAST,              "select-last")
ACTION(INPUT_JUMP_TO_NEXT,             "jump-to-next")
ACTION(INPUT_JUMP_TO_PREV,             "jump-to-prev")
ACTION(INPUT_JUMP_TO_NEXT_UNREAD,      "next-unread", "jump-to-next-unread")
ACTION(INPUT_JUMP_TO_PREV_UNREAD,      "prev-unread", "jump-to-prev-unread")
ACTION(INPUT_JUMP_TO_NEXT_IMPORTANT,   "next-important", "jump-to-next-important")
ACTION(INPUT_JUMP_TO_PREV_IMPORTANT,   "prev-important", "jump-to-prev-important")
ACTION(INPUT_JUMP_TO_NEXT_ERROR,       "next-error", "jump-to-next-error")
ACTION(INPUT_JUMP_TO_PREV_ERROR,       "prev-error", "jump-to-prev-error")
ACTION(INPUT_GOTO_FEED,                "goto-feed")
ACTION(INPUT_SHIFT_WEST,               "shift-west")
ACTION(INPUT_SHIFT_EAST,               "shift-east")
ACTION(INPUT_SHIFT_RESET,              "shift-reset")
ACTION(INPUT_SORT_BY_TIME,             "sort-by-time")
ACTION(INPUT_SORT_BY_TIME_DOWNLOAD,    "sort-by-time-download")
ACTION(INPUT_SORT_BY_TIME_PUBLICATION, "sort-by-time-publication")
ACTION(INPUT_SORT_BY_TIME_UPDATE,      "sort-by-time-update")
ACTION(INPUT_SORT_BY_ROWID,            "sort-by-rowid")
ACTION(INPUT_SORT_BY_UNREAD,           "sort-by-unread")
ACTION(INPUT_SORT_BY_INITIAL,          "sort-by-initial")
ACTION(INPUT_SORT_BY_ALPHABET,         "sort-by-alphabet")
ACTION(INPUT_SORT_BY_IMPORTANT,        "sort-by-important")
ACTION(INPUT_ENTER,                    "enter")
ACTION(INPUT_RELOAD,                   "reload")
ACTION(INPUT_RELOAD_ALL,               "reload-all")
ACTION(INPUT_MARK_READ,                "read", "mark-read")
ACTION(INPUT_MARK_UNREAD,              "unread", "mark-unread")
ACTION(INPUT_MARK_READ_ALL,            "read-all", "mark-read-all")
ACTION(INPUT_MARK_UNREAD_ALL,          "unread-all", "mark-unread-all")
ACTION(INPUT_MARK_IMPORTANT,           "important", "mark-important")
ACTION(INPUT_MARK_UNIMPORTANT,         "unimportant", "mark-unimportant")
ACTION(INPUT_TOGGLE_READ,              "toggle-read")
ACTION(INPUT_TOGGLE_IMPORTANT,         "toggle-important")
ACTION(INPUT_TOGGLE_EXPLORE_MODE,      "explore", "toggle-explore-mode")
ACTION(INPUT_TOGGLE_READ_FEEDS,        "toggle-read-feeds", "toggle-hide-read-feeds")
ACTION(INPUT_TOGGLE_READ_ITEMS,        "toggle-read-items", "toggle-hide-read-items")
ACTION(INPUT_VIEW_ERRORS,              "view-errors")
ACTION(INPUT_OPEN_IN_BROWSER,          "open-in-browser")
ACTION(INPUT_COPY_TO_CLIPBOARD,        "copy-to-clipboard")
ACTION(INPUT_START_SEARCH_INPUT,       "start-search-input")
ACTION(INPUT_CLEAN_STATUS,             "clean-status")
ACTION(INPUT_NAVIGATE_BACK,            "return", "navigate-back")
ACTION(INPUT_QUIT_SOFT,                "quit")
ACTION(INPUT_QUIT_HARD,                "quit-hard")
ACTION(INPUT_FIND_COMMAND,             "find")
ACTION(INPUT_SYSTEM_COMMAND,           "exec")
ACTION(INPUT_SYSTEM_COMMAND_QUIET,     "exec-quiet")
ACTION(INPUT_DATABASE_COMMAND,         "edit")
ACTION(INPUT_EMPTY)
ACTION(INPUT_ERROR)
ACTION(INPUT_APPLY_SEARCH_MODE_FILTER)
ACTION(INPUT_MAX)

#ifdef ACTION
};
#endif

#undef ACTION
