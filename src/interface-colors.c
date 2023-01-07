#include <curses.h>
#include "newsraft.h"

static bool paint_it_black = true;
static unsigned int attributes[NEWSRAFT_COLOR_PAIRS_COUNT];

bool
create_color_pairs(void)
{
	INFO("Creating color pairs.");
	for (size_t i = 0, j = 1; i < NEWSRAFT_COLOR_PAIRS_COUNT; ++i, j += 2) {
		int bg = get_cfg_color(i * 2 + 1, &attributes[i]);
		int fg = get_cfg_color(i * 2, &attributes[i]); // Set fg last because it bears the attributes.
		if ((init_pair(j, fg, bg) == ERR) || (init_pair(j + 1, bg, fg) == ERR)) {
			return false;
		}
	}
	paint_it_black = false;
	return true;
}

unsigned int
get_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_NORMAL : COLOR_PAIR(1 + id) | attributes[id / 2];
}

unsigned int
get_reversed_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_REVERSE : COLOR_PAIR(2 + id) | attributes[id / 2];
}
