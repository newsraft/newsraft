#include "newsraft.h"

static bool paint_it_black = true;
static unsigned int attributes[NEWSRAFT_COLOR_PAIRS_COUNT];

bool
create_color_pairs(void)
{
	INFO("Creating color pairs.");
	for (size_t i = 0; i < NEWSRAFT_COLOR_PAIRS_COUNT; ++i) {
		int bg = get_cfg_color(i * 2 + 1, &attributes[i]);
		int fg = get_cfg_color(i * 2, &attributes[i]); // Set fg last because it bears the attributes.
		if (init_pair(i + 1, fg, bg) == ERR) {
			return false;
		}
	}
	paint_it_black = false;
	return true;
}

unsigned int
get_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_NORMAL : COLOR_PAIR(id / 2 + 1) | attributes[id / 2];
}
