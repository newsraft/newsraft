#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H
#include "newsraft.h"

bool generate_useragent_string(struct string *target);

bool load_default_binds(void);

bool assign_default_values_to_null_config_strings(void);

void log_config_settings(void);

bool verify_config_values(void);

#endif // LOAD_CONFIG_H
