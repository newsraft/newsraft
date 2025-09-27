#include <string.h>
#include "newsraft.h"

struct sorting_bundle {
	const char *setting;
	const char *message;
};

static struct sorting_bundle sorting_methods[] = {
	[SORT_BY_INITIAL_ASC]           = {"initial-asc",           "Sorted %s according to initial order (ascending)"},
	[SORT_BY_INITIAL_DESC]          = {"initial-desc",          "Sorted %s according to initial order (descending)"},
	[SORT_BY_TIME_ASC]              = {"time-asc",              "Sorted %s by time (ascending)"},
	[SORT_BY_TIME_DESC]             = {"time-desc",             "Sorted %s by time (descending)"},
	[SORT_BY_TIME_DOWNLOAD_ASC]     = {"time-download-asc",     "Sorted %s by download time (ascending)"},
	[SORT_BY_TIME_DOWNLOAD_DESC]    = {"time-download-desc",    "Sorted %s by download time (descending)"},
	[SORT_BY_TIME_PUBLICATION_ASC]  = {"time-publication-asc",  "Sorted %s by publication time (ascending)"},
	[SORT_BY_TIME_PUBLICATION_DESC] = {"time-publication-desc", "Sorted %s by publication time (descending)"},
	[SORT_BY_TIME_UPDATE_ASC]       = {"time-update-asc",       "Sorted %s by update time (ascending)"},
	[SORT_BY_TIME_UPDATE_DESC]      = {"time-update-desc",      "Sorted %s by update time (descending)"},
	[SORT_BY_ROWID_ASC]             = {"rowid-asc",             "Sorted %s by rowid (ascending)"},
	[SORT_BY_ROWID_DESC]            = {"rowid-desc",            "Sorted %s by rowid (descending)"},
	[SORT_BY_UNREAD_ASC]            = {"unread-asc",            "Sorted %s by unread (ascending)"},
	[SORT_BY_UNREAD_DESC]           = {"unread-desc",           "Sorted %s by unread (descending)"},
	[SORT_BY_ALPHABET_ASC]          = {"alphabet-asc",          "Sorted %s in alphabetical order (ascending)"},
	[SORT_BY_ALPHABET_DESC]         = {"alphabet-desc",         "Sorted %s in alphabetical order (descending)"},
	[SORT_BY_IMPORTANT_ASC]         = {"important-asc",         "Sorted %s by importance (ascending)"},
	[SORT_BY_IMPORTANT_DESC]        = {"important-desc",        "Sorted %s by importance (descending)"},
};

sorting_method_t
get_sorting_id(const char *sorting_name)
{
	for (size_t i = 0; i < LENGTH(sorting_methods); ++i) {
		if (strcmp(sorting_name, sorting_methods[i].setting) == 0) {
			return i;
		}
	}
	return SORT_BY_INITIAL_ASC;
}

const char *
get_sorting_message(int sorting_id)
{
	return sorting_methods[sorting_id].message;
}
