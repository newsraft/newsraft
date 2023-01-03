#include <curses.h>
#include "newsraft.h"

struct color_assignment {
	unsigned int pair;
	unsigned int reverse_pair;
	unsigned int attribute;
};

static bool paint_it_black = true;
static struct color_assignment colors[NEWSRAFT_COLOR_PAIRS_COUNT];

bool
create_color_pairs(void)
{
	INFO("Creating color pairs.");
	int fg, bg;
	for (size_t i = 0, j = 1; i < NEWSRAFT_COLOR_PAIRS_COUNT; ++i, j += 2) {
		get_cfg_color(i * 2 + 1, &bg, &colors[i].attribute);
		get_cfg_color(i * 2, &fg, &colors[i].attribute);
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

unsigned int
get_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_NORMAL : colors[id / 2].pair | colors[id / 2].attribute;
}

unsigned int
get_reversed_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_REVERSE : colors[id / 2].reverse_pair | colors[id / 2].attribute;
}
