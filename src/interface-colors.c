#include <curses.h>
#include "newsraft.h"

struct color_assignment {
	int pair;
	int reverse_pair;
};

static bool paint_it_black = true; // Maybe then I'll fade away and not have to face the facts...
static struct color_assignment colors[NEWSRAFT_COLOR_PAIRS_COUNT];

bool
create_color_pairs(void)
{
	INFO("Creating color pairs.");
	int fg, bg;
	for (size_t i = 0, j = 1; i < NEWSRAFT_COLOR_PAIRS_COUNT; ++i, j += 2) {
		fg = get_cfg_color(i * 2);
		bg = get_cfg_color(i * 2 + 1);
		if (init_pair(j, fg, bg) == ERR) {
			return false;
		}
		colors[i].pair = COLOR_PAIR(j);
		if (init_pair(j + 1, bg, fg) == ERR) {
			return false;
		}
		colors[i].reverse_pair = COLOR_PAIR(j + 1);
	}
	paint_it_black = false;
	return true;
}

int
get_color_pair(config_entry_id id)
{
	return paint_it_black == true ? (int)A_NORMAL : colors[id / 2].pair;
}

int
get_reversed_color_pair(config_entry_id id)
{
	return paint_it_black == true ? (int)A_REVERSE : colors[id / 2].reverse_pair;
}
