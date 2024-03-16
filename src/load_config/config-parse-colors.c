#include <string.h>
#include "load_config/load_config.h"

bool
parse_color_setting(struct config_context **ctx, config_entry_id id, const char *iter)
{
	int colors[2] = {-1, -1}; // colors[0] = fg; colors[1] = bg;
	unsigned int c = 0;
	unsigned int attribute = A_NORMAL;
	while (*iter != '\0') {
		while (ISWHITESPACE(*iter)) iter += 1; // Skip whitespace.
		if      (strncmp(iter, "default",     7) == 0) { colors[c] = -1;            c ^= 1; }
		else if (strncmp(iter, "black",       5) == 0) { colors[c] = COLOR_BLACK;   c ^= 1; }
		else if (strncmp(iter, "red",         3) == 0) { colors[c] = COLOR_RED;     c ^= 1; }
		else if (strncmp(iter, "green",       5) == 0) { colors[c] = COLOR_GREEN;   c ^= 1; }
		else if (strncmp(iter, "yellow",      6) == 0) { colors[c] = COLOR_YELLOW;  c ^= 1; }
		else if (strncmp(iter, "blue",        4) == 0) { colors[c] = COLOR_BLUE;    c ^= 1; }
		else if (strncmp(iter, "magenta",     7) == 0) { colors[c] = COLOR_MAGENTA; c ^= 1; }
		else if (strncmp(iter, "cyan",        4) == 0) { colors[c] = COLOR_CYAN;    c ^= 1; }
		else if (strncmp(iter, "white",       5) == 0) { colors[c] = COLOR_WHITE;   c ^= 1; }
		else if (strncmp(iter, "bold",        4) == 0) { attribute |= A_BOLD;               }
		else if (strncmp(iter, "underlined", 10) == 0) { attribute |= A_UNDERLINE;          }
#ifdef A_ITALIC
		else if (strncmp(iter, "italic",      6) == 0) { attribute |= A_ITALIC;             }
#endif
		else if (strncmp(iter, "color",       5) == 0) {
			colors[c] = strtol(iter + 5, NULL, 10);
			if (colors[c] < 0 || colors[c] > 255) {
				fputs("Color number must be in the range from 0 to 255!\n", stderr);
				return false;
			}
			c ^= 1;
		} else {
			fputs("Color settings can only contain the following tokens:\n", stderr);
			fputs("default, black, red, green, yellow, blue, magenta, cyan, white, colorN,\n", stderr);
			fputs("bold, italic, underlined.\n", stderr);
			return false;
		}
		while (!ISWHITESPACE(*iter) && *iter != '\0') iter += 1; // Advance to next token.
	}
	set_cfg_color(ctx, id, colors[0], colors[1], attribute);
	return true;
}
