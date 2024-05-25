#include <string.h>
#include "load_config/load_config.h"

struct input_entry_match {
	const char *name;
	const input_id value;
};

static const struct input_entry_match actions_table[] = {
	{"select-next",            INPUT_SELECT_NEXT           },
	{"select-prev",            INPUT_SELECT_PREV           },
	{"select-next-page",       INPUT_SELECT_NEXT_PAGE      },
	{"select-prev-page",       INPUT_SELECT_PREV_PAGE      },
	{"select-first",           INPUT_SELECT_FIRST          },
	{"select-last",            INPUT_SELECT_LAST           },
	{"jump-to-next",           INPUT_JUMP_TO_NEXT          },
	{"jump-to-prev",           INPUT_JUMP_TO_PREV          },
	{"next-unread",            INPUT_JUMP_TO_NEXT_UNREAD   },
	{"jump-to-next-unread",    INPUT_JUMP_TO_NEXT_UNREAD   },
	{"prev-unread",            INPUT_JUMP_TO_PREV_UNREAD   },
	{"jump-to-prev-unread",    INPUT_JUMP_TO_PREV_UNREAD   },
	{"next-important",         INPUT_JUMP_TO_NEXT_IMPORTANT},
	{"jump-to-next-important", INPUT_JUMP_TO_NEXT_IMPORTANT},
	{"prev-important",         INPUT_JUMP_TO_PREV_IMPORTANT},
	{"jump-to-prev-important", INPUT_JUMP_TO_PREV_IMPORTANT},
	{"goto-feed",              INPUT_GOTO_FEED             },
	{"shift-west",             INPUT_SHIFT_WEST            },
	{"shift-east",             INPUT_SHIFT_EAST            },
	{"shift-reset",            INPUT_SHIFT_RESET           },
	{"sort-by-time",           INPUT_SORT_BY_TIME          },
	{"sort-by-unread",         INPUT_SORT_BY_UNREAD        },
	{"sort-by-alphabet",       INPUT_SORT_BY_ALPHABET      },
	{"sort-by-important",      INPUT_SORT_BY_IMPORTANT     },
	{"enter",                  INPUT_ENTER                 },
	{"reload",                 INPUT_RELOAD                },
	{"reload-all",             INPUT_RELOAD_ALL            },
	{"read",                   INPUT_MARK_READ             },
	{"mark-read",              INPUT_MARK_READ             },
	{"unread",                 INPUT_MARK_UNREAD           },
	{"mark-unread",            INPUT_MARK_UNREAD           },
	{"mark-read-all",          INPUT_MARK_READ_ALL         },
	{"mark-unread-all",        INPUT_MARK_UNREAD_ALL       },
	{"important",              INPUT_MARK_IMPORTANT        },
	{"mark-important",         INPUT_MARK_IMPORTANT        },
	{"unimportant",            INPUT_MARK_UNIMPORTANT      },
	{"mark-unimportant",       INPUT_MARK_UNIMPORTANT      },
	{"explore",                INPUT_TOGGLE_EXPLORE_MODE   },
	{"toggle-explore-mode",    INPUT_TOGGLE_EXPLORE_MODE   },
	{"status-history-menu",    INPUT_STATUS_HISTORY_MENU   },
	{"open-in-browser",        INPUT_OPEN_IN_BROWSER       },
	{"copy-to-clipboard",      INPUT_COPY_TO_CLIPBOARD     },
	{"start-search-input",     INPUT_START_SEARCH_INPUT    },
	{"navigate-back",          INPUT_NAVIGATE_BACK         },
	{"quit",                   INPUT_QUIT_SOFT             },
	{"quit-hard",              INPUT_QUIT_HARD             },
	{NULL,                     0                           },
};

input_id
get_input_id_by_name(const char *name)
{
	for (size_t i = 0; actions_table[i].name != NULL; ++i) {
		if (strcmp(name, actions_table[i].name) == 0) {
			return actions_table[i].value;
		}
	}
	write_error("Action \"%s\" doesn't exist!\n", name);
	return INPUT_ERROR;
}
