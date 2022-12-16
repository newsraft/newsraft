#include <string.h>
#include "load_config/load_config.h"

struct input_entry_match {
	const char *name;
	const input_cmd_id value;
};

static const struct input_entry_match entries[] = {
	{"select-next",           INPUT_SELECT_NEXT          },
	{"select-prev",           INPUT_SELECT_PREV          },
	{"select-next-unread",    INPUT_SELECT_NEXT_UNREAD   },
	{"select-prev-unread",    INPUT_SELECT_PREV_UNREAD   },
	{"select-next-important", INPUT_SELECT_NEXT_IMPORTANT},
	{"select-prev-important", INPUT_SELECT_PREV_IMPORTANT},
	{"select-next-page",      INPUT_SELECT_NEXT_PAGE     },
	{"select-prev-page",      INPUT_SELECT_PREV_PAGE     },
	{"select-first",          INPUT_SELECT_FIRST         },
	{"select-last",           INPUT_SELECT_LAST          },
	{"enter",                 INPUT_ENTER                },
	{"reload",                INPUT_RELOAD               },
	{"reload-all",            INPUT_RELOAD_ALL           },
	{"mark-read",             INPUT_MARK_READ            },
	{"mark-unread",           INPUT_MARK_UNREAD          },
	{"mark-read-all",         INPUT_MARK_READ_ALL        },
	{"mark-unread-all",       INPUT_MARK_UNREAD_ALL      },
	{"mark-important",        INPUT_MARK_IMPORTANT       },
	{"mark-unimportant",      INPUT_MARK_UNIMPORTANT     },
	{"explore-menu",          INPUT_EXPLORE_MENU         },
	{"status-history-menu",   INPUT_STATUS_HISTORY_MENU  },
	{"open-in-browser",       INPUT_OPEN_IN_BROWSER      },
	{"copy-to-clipboard",     INPUT_COPY_TO_CLIPBOARD    },
	{"quit",                  INPUT_QUIT_SOFT            },
	{"quit-hard",             INPUT_QUIT_HARD            },
	{NULL,                    0                          },
};

input_cmd_id
get_input_cmd_id_by_name(const char *name)
{
	for (uint8_t i = 0; entries[i].name != NULL; ++i) {
		if (strcmp(name, entries[i].name) == 0) {
			return entries[i].value;
		}
	}
	return INPUT_ERROR;
}
