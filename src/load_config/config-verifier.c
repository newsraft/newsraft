#include <stdio.h>
#include <string.h>
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
	return success;
}

bool
verify_config_values(void)
{
	bool success = true;
	if (get_cfg_uint(CFG_SIZE_CONVERSION_THRESHOLD) < 1000U) {
		success = false;
		fputs("Size conversion threshold must be greater than or equal to 1000!\n", stderr);
	}
	const struct string *str = get_cfg_string(CFG_PROXY);
	if ((str->len != 0) && (strstr(str->ptr, "://") == NULL)) {
		success = false;
		fputs("The proxy string must be prefixed with scheme:// to specify which kind of proxy is used!\n", stderr);
	}
	if (verify_format_string_lengths() == false) {
		success = false;
		fprintf(stderr, "Format string length must not exceed %d!\n", FORMAT_STRING_LENGTH_LIMIT);
	}
	return success;
}
