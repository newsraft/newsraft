#ifndef INPUT_H
#define INPUT_H

struct input_entry {
	const char *names[10];
	const char *default_binds[10];
};

#define INPUT(NAME, ...)  NAME,
enum {

#endif // INPUT_H

#ifdef INPUT_ARRAY

#define INPUT(NAME, ...)  [NAME] = {__VA_ARGS__},
static struct input_entry inputs[] = {

#endif // INPUT_ARRAY

INPUT(INPUT_SELECT_NEXT,              {"select-next"},                              {"j", "KEY_DOWN", "^E"})
INPUT(INPUT_SELECT_PREV,              {"select-prev"},                              {"k", "KEY_UP", "^Y"})
INPUT(INPUT_SELECT_NEXT_PAGE,         {"select-next-page"},                         {"space", "^F", "KEY_NPAGE"})
INPUT(INPUT_SELECT_NEXT_PAGE_HALF,    {"select-next-page-half"},                    {"^D"})
INPUT(INPUT_SELECT_PREV_PAGE,         {"select-prev-page"},                         {"^B", "KEY_PPAGE"})
INPUT(INPUT_SELECT_PREV_PAGE_HALF,    {"select-prev-page-half"},                    {"^U"})
INPUT(INPUT_SELECT_FIRST,             {"select-first"},                             {"g", "KEY_HOME"})
INPUT(INPUT_SELECT_LAST,              {"select-last"},                              {"G", "KEY_END"})
INPUT(INPUT_JUMP_TO_NEXT,             {"jump-to-next"},                             {"J"})
INPUT(INPUT_JUMP_TO_PREV,             {"jump-to-prev"},                             {"K"})
INPUT(INPUT_JUMP_TO_NEXT_UNREAD,      {"next-unread",    "jump-to-next-unread"},    {"n"})
INPUT(INPUT_JUMP_TO_PREV_UNREAD,      {"prev-unread",    "jump-to-prev-unread"},    {"N"})
INPUT(INPUT_JUMP_TO_NEXT_IMPORTANT,   {"next-important", "jump-to-next-important"}, {"p"})
INPUT(INPUT_JUMP_TO_PREV_IMPORTANT,   {"prev-important", "jump-to-prev-important"}, {"P"})
INPUT(INPUT_JUMP_TO_NEXT_ERROR,       {"next-error",     "jump-to-next-error"},     {"e"})
INPUT(INPUT_JUMP_TO_PREV_ERROR,       {"prev-error",     "jump-to-prev-error"},     {"E"})
INPUT(INPUT_GOTO_FEED,                {"goto-feed"},                                {"*"})
INPUT(INPUT_SHIFT_WEST,               {"shift-west"},                               {","})
INPUT(INPUT_SHIFT_EAST,               {"shift-east"},                               {"."})
INPUT(INPUT_SHIFT_RESET,              {"shift-reset"},                              {"<"})
INPUT(INPUT_SORT_BY_TIME,             {"sort-by-time"},                             {"t"})
INPUT(INPUT_SORT_BY_TIME_UPDATE,      {"sort-by-time-update"},                      {/* not set by default */})
INPUT(INPUT_SORT_BY_TIME_PUBLICATION, {"sort-by-time-publication"},                 {/* not set by default */})
INPUT(INPUT_SORT_BY_ROWID,            {"sort-by-rowid"},                            {"w"})
INPUT(INPUT_SORT_BY_UNREAD,           {"sort-by-unread"},                           {"u"})
INPUT(INPUT_SORT_BY_INITIAL,          {"sort-by-initial"},                          {"z"})
INPUT(INPUT_SORT_BY_ALPHABET,         {"sort-by-alphabet"},                         {"a"})
INPUT(INPUT_SORT_BY_IMPORTANT,        {"sort-by-important"},                        {"i"})
INPUT(INPUT_ENTER,                    {"enter"},                                    {"l", "enter", "KEY_RIGHT", "KEY_ENTER"})
INPUT(INPUT_RELOAD,                   {"reload"},                                   {"r"})
INPUT(INPUT_RELOAD_ALL,               {"reload-all"},                               {"R", "^R"})
INPUT(INPUT_MARK_READ,                {"read",        "mark-read"},                 {/* see assign_default_binds() */})
INPUT(INPUT_MARK_UNREAD,              {"unread",      "mark-unread"},               {/* see assign_default_binds() */})
INPUT(INPUT_MARK_READ_ALL,            {"read-all",    "mark-read-all"},             {"A"})
INPUT(INPUT_MARK_UNREAD_ALL,          {"unread-all",  "mark-unread-all"},           {/* not set by default */})
INPUT(INPUT_MARK_IMPORTANT,           {"important",   "mark-important"},            {"f"})
INPUT(INPUT_MARK_UNIMPORTANT,         {"unimportant", "mark-unimportant"},          {"F"})
INPUT(INPUT_TOGGLE_EXPLORE_MODE,      {"explore",     "toggle-explore-mode"},       {"tab"})
INPUT(INPUT_VIEW_ERRORS,              {"view-errors"},                              {"v"})
INPUT(INPUT_OPEN_IN_BROWSER,          {"open-in-browser"},                          {"o"})
INPUT(INPUT_COPY_TO_CLIPBOARD,        {"copy-to-clipboard"},                        {"y", "c"})
INPUT(INPUT_START_SEARCH_INPUT,       {"start-search-input"},                       {"/"})
INPUT(INPUT_CLEAN_STATUS,             {"clean-status"},                             {"escape"})
INPUT(INPUT_NAVIGATE_BACK,            {"return", "navigate-back"},                  {"h", "backspace", "KEY_LEFT", "KEY_BACKSPACE"})
INPUT(INPUT_QUIT_SOFT,                {"quit"},                                     {"q"})
INPUT(INPUT_QUIT_HARD,                {"quit-hard"},                                {"Q"})
INPUT(INPUT_FIND_COMMAND,             {"find"},                                     {})
INPUT(INPUT_SYSTEM_COMMAND,           {"exec"},                                     {})
INPUT(INPUT_DATABASE_COMMAND,         {"edit"},                                     {})
INPUT(INPUT_ERROR,                    {},                                           {})
INPUT(INPUT_APPLY_SEARCH_MODE_FILTER, {},                                           {})

#ifdef INPUT
};
#endif

#undef INPUT
