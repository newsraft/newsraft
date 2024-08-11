#include "newsraft.h"

// Note to the future.
// Since Ncurses has a limit on a number of color pairs, we must initialize
// them on an on demand basis to avoid initializing colors which may not appear
// on the screen at all and take up a precious color pair place in vain.

struct newsraft_color_pair {
	int pair_index;
	int video_attribute;
	int foreground;
	int background;
};

int
get_color_pair_unprotected(int fg, int bg)
{
	static struct newsraft_color_pair *pairs = NULL;
	static size_t pairs_count = 0;
	static int ncurses_pair_iterator = 1;

	if (!arent_we_colorful()) {
		return A_NORMAL;
	}

	for (size_t i = 0; i < pairs_count; ++i) {
		if (fg == pairs[i].foreground && bg == pairs[i].background) {
			if (pairs[i].pair_index > 0) {
				return pairs[i].video_attribute;
			}
			if (init_pair(ncurses_pair_iterator, fg, bg) == OK) {
				pairs[i].pair_index = ncurses_pair_iterator;
				pairs[i].video_attribute = COLOR_PAIR(pairs[i].pair_index);
				ncurses_pair_iterator += 1;
				return pairs[i].video_attribute;
			}
			WARN("Failed to initialize color pair (%d, %d)", fg, bg);
			// Color pair 0 is a special color pair which denotes no color.
			return COLOR_PAIR(0);
		}
	}

	void *tmp = realloc(pairs, sizeof(struct newsraft_color_pair) * (pairs_count + 1));
	if (tmp == NULL) {
		FAIL("Not enough memory for a color pair!\n");
		return COLOR_PAIR(0);
	}

	pairs = tmp;
	pairs[pairs_count].pair_index = -1;
	pairs[pairs_count].foreground = fg;
	pairs[pairs_count].background = bg;
	pairs_count += 1;

	return get_color_pair_unprotected(fg, bg);
}
