#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H
#include "newsraft.h"

typedef uint8_t config_type_id;
enum config_type {
	CFG_BOOL,
	CFG_UINT,
	CFG_COLOR,
	CFG_STRING,
};

// See "config.c" file for implementation.
config_entry_id find_config_entry_by_name(const char *name);
bool prepare_config_string_settings(void);
config_type_id get_cfg_type(config_entry_id i);
void set_cfg_bool(config_entry_id i, bool value);
void set_cfg_uint(config_entry_id i, size_t value);
void set_cfg_color_hue(config_entry_id i, int hue);
void set_cfg_color_attribute(config_entry_id i, unsigned int attribute);
bool set_cfg_string(config_entry_id i, const char *src_ptr, size_t src_len);

// See "config-auto.c" file for implementation.
bool obtain_useragent_string(struct string **ua);
bool obtain_clipboard_command(struct string **cmd);
bool obtain_notification_command(struct string **cmd);

void log_config_settings(void);

input_cmd_id get_input_cmd_id_by_name(const char *name);

bool parse_color_setting(config_entry_id id, const char *iter);
#endif // LOAD_CONFIG_H
