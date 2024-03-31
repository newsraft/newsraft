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

// See "load_config.c" file for implementation.
config_entry_id find_config_entry_by_name(const char *name);
config_type_id get_cfg_type(config_entry_id id);
void set_cfg_bool(struct config_context **ctx, config_entry_id id, bool value);
void set_cfg_uint(struct config_context **ctx, config_entry_id id, size_t value);
void set_cfg_color(struct config_context **ctx, config_entry_id id, int fg, int bg, unsigned int attribute);
bool set_cfg_string(struct config_context **ctx, config_entry_id id, const char *src_ptr, size_t src_len);

// See "config-auto.c" file for implementation.
bool obtain_useragent_string(struct config_context **ctx, config_type_id id);
bool obtain_clipboard_command(struct config_context **ctx, config_type_id id);
bool obtain_notification_command(struct config_context **ctx, config_type_id id);

input_id get_input_id_by_name(const char *name);

bool parse_color_setting(struct config_context **ctx, config_entry_id id, const char *iter);
#endif // LOAD_CONFIG_H
