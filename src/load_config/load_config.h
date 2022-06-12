#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H
#include "newsraft.h"

typedef uint8_t config_type_id;
enum config_type {
	CFG_BOOL,
	CFG_UINT,
	CFG_STRING,
	CFG_WSTRING,
};

config_entry_id find_config_entry_by_name(const char *name);

bool parse_config_file(const char *path);

bool generate_useragent_string(struct string *target);
bool generate_copy_to_clipboard_command_string(struct string *cmd);

bool assign_default_values_to_null_config_strings(void);
bool assign_calculated_values_to_auto_config_strings(void);

config_type_id get_cfg_type(size_t i);
void set_cfg_bool(size_t i, bool value);
void set_cfg_uint(size_t i, size_t value);
bool set_cfg_string(size_t i, const struct string *value);
bool set_cfg_wstring(size_t i, const struct string *value);

void log_config_settings(void);

bool verify_config_values(void);

#endif // LOAD_CONFIG_H
