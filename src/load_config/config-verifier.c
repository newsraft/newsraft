#include <stdio.h>
#include "load_config/load_config.h"

static inline bool
verify_format_string_lengths(void)
{
	bool success = true;
	const struct wstring *wstr = get_cfg_wstring(CFG_MENU_SECTION_ENTRY_FORMAT);
	if (wstr->len > FORMAT_STRING_LENGTH_LIMIT) {
		success = false;
		fprintf(stderr, "%s is too long!\n", get_cfg_name(CFG_MENU_SECTION_ENTRY_FORMAT));
	}
	wstr = get_cfg_wstring(CFG_MENU_FEED_ENTRY_FORMAT);
	if (wstr->len > FORMAT_STRING_LENGTH_LIMIT) {
		success = false;
		fprintf(stderr, "%s is too long!\n", get_cfg_name(CFG_MENU_FEED_ENTRY_FORMAT));
	}
	wstr = get_cfg_wstring(CFG_MENU_ITEM_ENTRY_FORMAT);
	if (wstr->len > FORMAT_STRING_LENGTH_LIMIT) {
		success = false;
		fprintf(stderr, "%s is too long!\n", get_cfg_name(CFG_MENU_ITEM_ENTRY_FORMAT));
	}
	if (success == false) {
		fprintf(stderr, "Format string length must not exceed %d!\n", FORMAT_STRING_LENGTH_LIMIT);
	}
	return success;
}

bool
verify_config_values(void)
{
	return verify_format_string_lengths();
}
