#include <string.h>
#include <curses.h>
#include "load_config/load_config.h"

struct color_token {
	const char *name;
	const size_t len;
	const bool is_color;
	const int code;
};

static const struct color_token tokens[] = {
	{"default",    7,  true,  -1},
	{"black",      5,  true,  COLOR_BLACK},
	{"red",        3,  true,  COLOR_RED},
	{"green",      5,  true,  COLOR_GREEN},
	{"yellow",     6,  true,  COLOR_YELLOW},
	{"blue",       4,  true,  COLOR_BLUE},
	{"magenta",    7,  true,  COLOR_MAGENTA},
	{"cyan",       4,  true,  COLOR_CYAN},
	{"white",      5,  true,  COLOR_WHITE},
	{"bold",       4,  false, A_BOLD},
#ifdef A_ITALIC
	{"italic",     6,  false, A_ITALIC},
#endif
	{"underlined", 10, false, A_UNDERLINE},
	{NULL,         0,  false, 0},
};

bool
parse_color_setting(config_entry_id id, const char *iter)
{
	size_t i;
	unsigned int attribute = A_NORMAL;
	while (*iter != '\0') {
		for (i = 0; tokens[i].name != NULL; ++i) {
			if (strncmp(iter, tokens[i].name, tokens[i].len) == 0) {
				if ((!ISWHITESPACE(iter[tokens[i].len])) && (iter[tokens[i].len] != '\0')) {
					goto invalid;
				}
				if (tokens[i].is_color == true) {
					set_cfg_color_hue(id, tokens[i].code);
				} else {
					if ((id % 2) != 0) {
						fputs("Color attributes can be set only for foreground color settings!\n", stderr);
						return false;
					}
					attribute |= tokens[i].code;
				}
				break;
			}
		}
		if (tokens[i].name == NULL) {
			goto invalid;
		}
		iter += tokens[i].len;
		while (ISWHITESPACE(*iter)) {
			iter += 1;
		}
	}
	set_cfg_color_attribute(id, attribute);
	return true;
invalid:
	fputs("Color settings can only contain the following tokens:\n", stderr);
	fputs("black, red, green, yellow, blue, magenta, cyan, white,\n", stderr);
	fputs("bold, italic, underlined.\n", stderr);
	return false;
}
