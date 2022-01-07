#include "update_feed/update_feed.h"

bool
db_bind_text_struct(sqlite3_stmt *s, intmax_t placeholder, const struct text *text_struct)
{
	if (text_struct->value->len == 0) {
		sqlite3_bind_text(s, placeholder, "", 0, NULL);
		return true;
	}
	if (catcs(text_struct->type, ';') == false) {
		return false;
	}
	if (catss(text_struct->type, text_struct->value) == false) {
		return false;
	}
	if (sqlite3_bind_text(s, placeholder, text_struct->type->ptr, text_struct->type->len, NULL) != SQLITE_OK) {
		return false;
	}
	return true;
}
