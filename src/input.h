#ifndef INPUT_H
#define INPUT_H

struct input_entry {
	const char *default_binds[10];
	const char *names[10];
};

#define INPUT(NAME, DEFAULT_BINDS, DEFAULT_STRINGS)  NAME,
enum {

#endif // INPUT_H

#ifdef INPUT_ARRAY

#define WRAP(...) __VA_ARGS__
#define INPUT(NAME, DEFAULT_BIND, DEFAULT_STRING)  [NAME] = {DEFAULT_BIND, DEFAULT_STRING},
static struct input_entry inputs[] = {

#endif // INPUT_ARRAY

INPUT(INPUT_SELECT_NEXT,              WRAP({"j", "KEY_DOWN", "^E"}),                  WRAP({"select-next"}))
INPUT(INPUT_SELECT_PREV,              WRAP({"k", "KEY_UP", "^Y"}),                    WRAP({"select-prev"}))
INPUT(INPUT_SELECT_NEXT_PAGE,         WRAP({"space", "^F", "KEY_NPAGE"}),             WRAP({"select-next-page"}))
INPUT(INPUT_SELECT_PREV_PAGE,         WRAP({"^B", "KEY_PPAGE", }),                    WRAP({"select-prev-page"}))
INPUT(INPUT_SELECT_FIRST,             WRAP({"g", "KEY_HOME"}),                        WRAP({"select-first"}))
INPUT(INPUT_SELECT_LAST,              WRAP({"G", "KEY_END"}),                         WRAP({"select-last"}))
INPUT(INPUT_JUMP_TO_NEXT,             WRAP({"J"}),                                    WRAP({"jump-to-next"}))
INPUT(INPUT_JUMP_TO_PREV,             WRAP({"K"}),                                    WRAP({"jump-to-prev"}))
INPUT(INPUT_JUMP_TO_NEXT_UNREAD,      WRAP({"n"}),                                    WRAP({"next-unread",    "jump-to-next-unread"}))
INPUT(INPUT_JUMP_TO_PREV_UNREAD,      WRAP({"N"}),                                    WRAP({"prev-unread",    "jump-to-prev-unread"}))
INPUT(INPUT_JUMP_TO_NEXT_IMPORTANT,   WRAP({"p"}),                                    WRAP({"next-important", "jump-to-next-important"}))
INPUT(INPUT_JUMP_TO_PREV_IMPORTANT,   WRAP({"P"}),                                    WRAP({"prev-important", "jump-to-prev-important"}))
INPUT(INPUT_GOTO_FEED,                WRAP({"*"}),                                    WRAP({"goto-feed"}))
INPUT(INPUT_SHIFT_WEST,               WRAP({","}),                                    WRAP({"shift-west"}))
INPUT(INPUT_SHIFT_EAST,               WRAP({"."}),                                    WRAP({"shift-east"}))
INPUT(INPUT_SHIFT_RESET,              WRAP({"<"}),                                    WRAP({"shift-reset"}))
INPUT(INPUT_SORT_BY_TIME,             WRAP({"t"}),                                    WRAP({"sort-by-time"}))
INPUT(INPUT_SORT_BY_UNREAD,           WRAP({"u"}),                                    WRAP({"sort-by-unread"}))
INPUT(INPUT_SORT_BY_ALPHABET,         WRAP({"a"}),                                    WRAP({"sort-by-alphabet"}))
INPUT(INPUT_SORT_BY_IMPORTANT,        WRAP({"i"}),                                    WRAP({"sort-by-important"}))
INPUT(INPUT_ENTER,                    WRAP({"l", "enter", "KEY_RIGHT", "KEY_ENTER"}), WRAP({"enter"}))
INPUT(INPUT_RELOAD,                   WRAP({"r"}),                                    WRAP({"reload"}))
INPUT(INPUT_RELOAD_ALL,               WRAP({"R", "^R"}),                              WRAP({"reload-all"}))
INPUT(INPUT_MARK_READ,                WRAP({}), /* look at assign_default_binds() */  WRAP({"read",        "mark-read"}))
INPUT(INPUT_MARK_UNREAD,              WRAP({}), /* look at assign_default_binds() */  WRAP({"unread",      "mark-unread"}))
INPUT(INPUT_MARK_READ_ALL,            WRAP({"^D"}),                                   WRAP({"read-all",    "mark-read-all"}))
INPUT(INPUT_MARK_UNREAD_ALL,          WRAP({}), /* basically unset */                 WRAP({"unread-all",  "mark-unread-all"}))
INPUT(INPUT_MARK_IMPORTANT,           WRAP({"f"}),                                    WRAP({"important",   "mark-important"}))
INPUT(INPUT_MARK_UNIMPORTANT,         WRAP({"F"}),                                    WRAP({"unimportant", "mark-unimportant"}))
INPUT(INPUT_TOGGLE_EXPLORE_MODE,      WRAP({"tab", "e"}),                             WRAP({"explore",     "toggle-explore-mode"}))
INPUT(INPUT_STATUS_HISTORY_MENU,      WRAP({"v"}),                                    WRAP({"status-history-menu"}))
INPUT(INPUT_OPEN_IN_BROWSER,          WRAP({"o"}),                                    WRAP({"open-in-browser"}))
INPUT(INPUT_COPY_TO_CLIPBOARD,        WRAP({"y", "c"}),                               WRAP({"copy-to-clipboard"}))
INPUT(INPUT_START_SEARCH_INPUT,       WRAP({"/"}),                                    WRAP({"start-search-input"}))
INPUT(INPUT_CLEAN_STATUS,             WRAP({"escape"}),                               WRAP({"clean-status"}))
INPUT(INPUT_NAVIGATE_BACK,            WRAP({"h", "backspace", "KEY_LEFT", "KEY_BACKSPACE"}), WRAP({"return", "navigate-back"}))
INPUT(INPUT_QUIT_SOFT,                WRAP({"q"}),                                    WRAP({"quit"}))
INPUT(INPUT_QUIT_HARD,                WRAP({"Q"}),                                    WRAP({"quit-hard"}))
INPUT(INPUT_SYSTEM_COMMAND,           WRAP({}),                                       WRAP({}))
INPUT(INPUT_ERROR,                    WRAP({}),                                       WRAP({}))
INPUT(INPUT_APPLY_SEARCH_MODE_FILTER, WRAP({}),                                       WRAP({}))

#ifdef INPUT
};
#endif

#undef INPUT
