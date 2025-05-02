#ifndef INPUT_H
#define INPUT_H

struct input_entry {
	const char *default_binds[10];
	const char *names[10];
};

#define INPUT(NAME, ...)  NAME,
enum {

#endif // INPUT_H

#ifdef INPUT_ARRAY

#define INPUT(NAME, ...)  [NAME] = {__VA_ARGS__},
static struct input_entry inputs[] = {

#endif // INPUT_ARRAY

INPUT(INPUT_SELECT_NEXT,              {"j", "KEY_DOWN", "^E"},                  {"select-next"})
INPUT(INPUT_SELECT_PREV,              {"k", "KEY_UP", "^Y"},                    {"select-prev"})
INPUT(INPUT_SELECT_NEXT_PAGE,         {"space", "^F", "KEY_NPAGE"},             {"select-next-page"})
INPUT(INPUT_SELECT_NEXT_PAGE_HALF,    {"^D"},                                   {"select-next-page-half"})
INPUT(INPUT_SELECT_PREV_PAGE,         {"^B", "KEY_PPAGE"},                      {"select-prev-page"})
INPUT(INPUT_SELECT_PREV_PAGE_HALF,    {"^U"},                                   {"select-prev-page-half"})
INPUT(INPUT_SELECT_FIRST,             {"g", "KEY_HOME"},                        {"select-first"})
INPUT(INPUT_SELECT_LAST,              {"G", "KEY_END"},                         {"select-last"})
INPUT(INPUT_JUMP_TO_NEXT,             {"J"},                                    {"jump-to-next"})
INPUT(INPUT_JUMP_TO_PREV,             {"K"},                                    {"jump-to-prev"})
INPUT(INPUT_JUMP_TO_NEXT_UNREAD,      {"n"},                                    {"next-unread",    "jump-to-next-unread"})
INPUT(INPUT_JUMP_TO_PREV_UNREAD,      {"N"},                                    {"prev-unread",    "jump-to-prev-unread"})
INPUT(INPUT_JUMP_TO_NEXT_IMPORTANT,   {"p"},                                    {"next-important", "jump-to-next-important"})
INPUT(INPUT_JUMP_TO_PREV_IMPORTANT,   {"P"},                                    {"prev-important", "jump-to-prev-important"})
INPUT(INPUT_JUMP_TO_NEXT_ERROR,       {"e"},                                    {"next-error",     "jump-to-next-error"})
INPUT(INPUT_JUMP_TO_PREV_ERROR,       {"E"},                                    {"prev-error",     "jump-to-prev-error"})
INPUT(INPUT_GOTO_FEED,                {"*"},                                    {"goto-feed"})
INPUT(INPUT_SHIFT_WEST,               {","},                                    {"shift-west"})
INPUT(INPUT_SHIFT_EAST,               {"."},                                    {"shift-east"})
INPUT(INPUT_SHIFT_RESET,              {"<"},                                    {"shift-reset"})
INPUT(INPUT_SORT_BY_TIME,             {"t"},                                    {"sort-by-time"})
INPUT(INPUT_SORT_BY_ROWID,            {"w"},                                    {"sort-by-rowid"})
INPUT(INPUT_SORT_BY_UNREAD,           {"u"},                                    {"sort-by-unread"})
INPUT(INPUT_SORT_BY_INITIAL,          {"z"},                                    {"sort-by-initial"})
INPUT(INPUT_SORT_BY_ALPHABET,         {"a"},                                    {"sort-by-alphabet"})
INPUT(INPUT_SORT_BY_IMPORTANT,        {"i"},                                    {"sort-by-important"})
INPUT(INPUT_ENTER,                    {"l", "enter", "KEY_RIGHT", "KEY_ENTER"}, {"enter"})
INPUT(INPUT_RELOAD,                   {"r"},                                    {"reload"})
INPUT(INPUT_RELOAD_ALL,               {"R", "^R"},                              {"reload-all"})
INPUT(INPUT_MARK_READ,                {}, /* look at assign_default_binds() */  {"read",        "mark-read"})
INPUT(INPUT_MARK_UNREAD,              {}, /* look at assign_default_binds() */  {"unread",      "mark-unread"})
INPUT(INPUT_MARK_READ_ALL,            {"A"},                                    {"read-all",    "mark-read-all"})
INPUT(INPUT_MARK_UNREAD_ALL,          {}, /* basically unset */                 {"unread-all",  "mark-unread-all"})
INPUT(INPUT_MARK_IMPORTANT,           {"f"},                                    {"important",   "mark-important"})
INPUT(INPUT_MARK_UNIMPORTANT,         {"F"},                                    {"unimportant", "mark-unimportant"})
INPUT(INPUT_TOGGLE_EXPLORE_MODE,      {"tab"},                                  {"explore",     "toggle-explore-mode"})
INPUT(INPUT_VIEW_ERRORS,              {"v"},                                    {"view-errors"})
INPUT(INPUT_OPEN_IN_BROWSER,          {"o"},                                    {"open-in-browser"})
INPUT(INPUT_COPY_TO_CLIPBOARD,        {"y", "c"},                               {"copy-to-clipboard"})
INPUT(INPUT_START_SEARCH_INPUT,       {"/"},                                    {"start-search-input"})
INPUT(INPUT_CLEAN_STATUS,             {"escape"},                               {"clean-status"})
INPUT(INPUT_NAVIGATE_BACK,            {"h", "backspace", "KEY_LEFT", "KEY_BACKSPACE"}, {"return", "navigate-back"})
INPUT(INPUT_QUIT_SOFT,                {"q"},                                    {"quit"})
INPUT(INPUT_QUIT_HARD,                {"Q"},                                    {"quit-hard"})
INPUT(INPUT_SYSTEM_COMMAND,           {},                                       {})
INPUT(INPUT_DATABASE_COMMAND,         {},                                       {})
INPUT(INPUT_ERROR,                    {},                                       {})
INPUT(INPUT_APPLY_SEARCH_MODE_FILTER, {},                                       {})

#ifdef INPUT
};
#endif

#undef INPUT
