#include "newsraft.h"

static bool paint_it_black = true;
static unsigned int attributes[NEWSRAFT_COLOR_PAIRS_COUNT];

bool
create_color_pairs(void)
{
	INFO("Creating color pairs.");
	int fg, bg;
	for (size_t i = 0; i < NEWSRAFT_COLOR_PAIRS_COUNT; ++i) {
		attributes[i] = get_cfg_color(i, &fg, &bg);
		if (init_pair(i + 1, fg, bg) == ERR) {
			WARN("Failed to create color pair fg=%d bg=%d, making this pair in regular colors...", fg, bg);
			if (init_pair(i + 1, -1, -1) == ERR) {
				FAIL("Failed to set troubled color pair to regular colors!");
				return false;
			}
		}
	}
	paint_it_black = false;
	return true;
}

unsigned int
get_color_pair(config_entry_id id)
{
	return paint_it_black == true ? A_NORMAL : COLOR_PAIR(id + 1) | attributes[id];
}
