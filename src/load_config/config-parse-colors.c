#include <stdlib.h>
#include <string.h>
#include "load_config/load_config.h"

bool
parse_color_setting(config_entry_id id, const char *iter)
{
	unsigned int attribute = A_NORMAL;
	while (*iter != '\0') {
		while (ISWHITESPACE(*iter)) iter += 1; // Skip whitespace.
		if      (strncmp(iter, "default", 7) == 0) { set_cfg_color_hue(id, -1);            }
		else if (strncmp(iter, "black",   5) == 0) { set_cfg_color_hue(id, COLOR_BLACK);   }
		else if (strncmp(iter, "red",     3) == 0) { set_cfg_color_hue(id, COLOR_RED);     }
		else if (strncmp(iter, "green",   5) == 0) { set_cfg_color_hue(id, COLOR_GREEN);   }
		else if (strncmp(iter, "yellow",  6) == 0) { set_cfg_color_hue(id, COLOR_YELLOW);  }
		else if (strncmp(iter, "blue",    4) == 0) { set_cfg_color_hue(id, COLOR_BLUE);    }
		else if (strncmp(iter, "magenta", 7) == 0) { set_cfg_color_hue(id, COLOR_MAGENTA); }
		else if (strncmp(iter, "cyan",    4) == 0) { set_cfg_color_hue(id, COLOR_CYAN);    }
		else if (strncmp(iter, "white",   5) == 0) { set_cfg_color_hue(id, COLOR_WHITE);   }
		else if (strncmp(iter, "color",   5) == 0) {
			long color_value = strtol(iter + 5, NULL, 10);
			if (color_value < 0 || color_value > 255) {
				fputs("Color number must be in the range from 0 to 255!\n", stderr);
				return false;
			}
			set_cfg_color_hue(id, color_value);
		} else if (strncmp(iter, "bold", 4) == 0 || strncmp(iter, "italic", 6) == 0 || strncmp(iter, "underlined", 10) == 0) {
			if ((id % 2) != 0) {
				fputs("Color attributes can be set only for foreground color settings!\n", stderr);
				return false;
			}
			if      (strncmp(iter, "bold",        4) == 0) { attribute |= A_BOLD;      }
#ifdef A_ITALIC
			else if (strncmp(iter, "italic",      6) == 0) { attribute |= A_ITALIC;    }
#endif
			else if (strncmp(iter, "underlined", 10) == 0) { attribute |= A_UNDERLINE; }
		} else {
			fputs("Color settings can only contain the following tokens:\n", stderr);
			fputs("default, black, red, green, yellow, blue, magenta, cyan, white, colorN,\n", stderr);
			fputs("bold, italic, underlined.\n", stderr);
			return false;
		}
		while (!ISWHITESPACE(*iter) && *iter != '\0') iter += 1; // Advance to next token.
	}
	set_cfg_color_attribute(id, attribute);
	return true;
}
