#include <string.h>
#include "load_config/load_config.h"

bool
parse_color_setting(struct config_context **ctx, config_entry_id id, const char *iter)
{
	long long colors[2] = {TB_DEFAULT, TB_DEFAULT}; // colors[0] = fg; colors[1] = bg;
	uintattr_t attribute = TB_DEFAULT;
	unsigned i = 0;
	while (*iter != '\0') {
		while (ISWHITESPACE(*iter)) iter += 1; // Skip whitespace
		if      (strncmp(iter, "default",     7) == 0) { colors[i] = TB_DEFAULT; i ^= 1; }
		else if (strncmp(iter, "black",       5) == 0) { colors[i] = TB_BLACK;   i ^= 1; }
		else if (strncmp(iter, "red",         3) == 0) { colors[i] = TB_RED;     i ^= 1; }
		else if (strncmp(iter, "green",       5) == 0) { colors[i] = TB_GREEN;   i ^= 1; }
		else if (strncmp(iter, "yellow",      6) == 0) { colors[i] = TB_YELLOW;  i ^= 1; }
		else if (strncmp(iter, "blue",        4) == 0) { colors[i] = TB_BLUE;    i ^= 1; }
		else if (strncmp(iter, "magenta",     7) == 0) { colors[i] = TB_MAGENTA; i ^= 1; }
		else if (strncmp(iter, "cyan",        4) == 0) { colors[i] = TB_CYAN;    i ^= 1; }
		else if (strncmp(iter, "white",       5) == 0) { colors[i] = TB_WHITE;   i ^= 1; }
		else if (strncmp(iter, "bold",        4) == 0) { attribute |= TB_BOLD;           }
		else if (strncmp(iter, "underlined", 10) == 0) { attribute |= TB_UNDERLINE;      }
		else if (strncmp(iter, "italic",      6) == 0) { attribute |= TB_ITALIC;         }
		else if (strncmp(iter, "color",       5) == 0) {
			tb_set_output_mode(TB_OUTPUT_256);
			colors[i] = strtoll(iter + 5, NULL, 10);
			if (colors[i] < 0 || colors[i] > 255) {
				write_error("Color number must be in the range from 0 to 255!\n");
				return false;
			}
			i ^= 1;
		} else {
			write_error("Color settings can only contain the following tokens:\n");
			write_error("default, black, red, green, yellow, blue, magenta, cyan, white, colorN,\n");
			write_error("bold, italic, underlined.\n");
			return false;
		}
		while (!ISWHITESPACE(*iter) && *iter != '\0') iter += 1; // Advance to next token
	}
	set_cfg_color(ctx, id, colors[0], colors[1], attribute);
	return true;
}
