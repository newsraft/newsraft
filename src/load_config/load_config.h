#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H
#include "newsraft.h"

typedef uint8_t config_type_id;
enum config_type {
	CFG_BOOL,
	CFG_UINT,
	CFG_COLOR,
	CFG_STRING,
	CFG_WSTRING,
};

config_entry_id find_config_entry_by_name(const char *name);

bool parse_config_file(const char *path);

void set_sane_value_for_update_threads_count(size_t initial_value);

bool generate_useragent_string(struct string *target);
bool generate_open_in_browser_command_string(struct string *cmd);
bool generate_copy_to_clipboard_command_string(struct string *cmd);

bool assign_default_values_to_null_config_strings(void);
bool assign_calculated_values_to_auto_config_strings(void);

config_type_id get_cfg_type(config_entry_id i);
void set_cfg_bool(config_entry_id i, bool value);
void set_cfg_uint(config_entry_id i, size_t value);
void set_cfg_color(config_entry_id i, int value);
bool set_cfg_string(config_entry_id i, const char *src_ptr, size_t src_len);
bool set_cfg_wstring(config_entry_id i, const char *src_ptr, size_t src_len);

void log_config_settings(void);

input_cmd_id get_input_cmd_id_by_name(const char *name);
#endif // LOAD_CONFIG_H
