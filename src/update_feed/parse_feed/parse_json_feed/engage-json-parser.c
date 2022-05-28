#include <string.h>
#include "update_feed/parse_feed/parse_json_feed/parse_json_feed.h"

static int
null_handler(void *ctx)
{
	(void)ctx;
	INFO("NULL here!");
	return 1;
}

static int
boolean_handler(void *ctx, int boolVal)
{
	(void)ctx;
	INFO("BOOL here: %d", boolVal);
	return 1;
}

static int
integer_handler(void *ctx, long long integerVal)
{
	(void)ctx;
	INFO("INT here: %lld", integerVal);
	return 1;
}

static int
double_handler(void *ctx, double doubleVal)
{
	(void)ctx;
	INFO("DBL here: %lf", doubleVal);
	return 1;
}

static int
number_handler(void *ctx, const char *numberVal, size_t numberLen)
{
	(void)ctx;
	INFO("NUM(string) here: %*s", (int)numberLen, numberVal);
	return 1;
}

static int
string_handler(void *ctx, const unsigned char *stringVal, size_t stringLen)
{
	(void)ctx;
	INFO("STR here: %*s", (int)stringLen, stringVal);
	return 1;
}

static int
start_map_handler(void *ctx)
{
	(void)ctx;
	INFO("STARTMAP here!");
	return 1;
}

static int
map_key_handler(void *ctx, const unsigned char *key, size_t stringLen)
{
	(void)ctx;
	INFO("KEYMAP here: %*s", (int)stringLen, key);
	return 1;
}

static int
end_map_handler(void *ctx)
{
	(void)ctx;
	INFO("ENDMAP here!");
	return 1;
}

static int
start_array_handler(void *ctx)
{
	(void)ctx;
	INFO("START ARRAY here!");
	return 1;
}

static int
end_array_handler(void *ctx)
{
	(void)ctx;
	INFO("END ARRAY here!");
	return 1;
}

static const yajl_callbacks callbacks = {
	&null_handler,
	&boolean_handler,
	&integer_handler,
	&double_handler,
	&number_handler,
	&string_handler,
	&start_map_handler,
	&map_key_handler,
	&end_map_handler,
	&start_array_handler,
	&end_array_handler
};

bool
engage_json_parser(struct stream_callback_data *data)
{
	data->json_parser = yajl_alloc(&callbacks, NULL, data);
	if (data->json_parser == NULL) {
		return false;
	}
	return true;
}

void
free_json_parser(struct stream_callback_data *data)
{
	yajl_complete_parse(data->json_parser); // final call
	yajl_free(data->json_parser);
}
