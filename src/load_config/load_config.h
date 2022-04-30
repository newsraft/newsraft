#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H
#include "feedeater.h"

struct string *generate_useragent_string(void);

bool assign_default_values_to_config_settings(void);
bool load_default_binds(void);

bool verify_config_values(void);

#endif // LOAD_CONFIG_H
