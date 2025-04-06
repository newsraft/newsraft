#include <string.h>
#include "newsraft.h"

static sqlite3 *db;

bool
db_init(void)
{
	const char *path = get_db_path();
	if (path == NULL) {
		// Error message written by get_db_path.
		return false;
	}

	if (sqlite3_config(SQLITE_CONFIG_SERIALIZED) != SQLITE_OK) {
		write_error("Failed to set threading mode of database to serialized!\n");
		write_error("It probably happens because SQLite was compiled without multithreading functionality.\n");
		return false;
	}

	if (sqlite3_open(path, &db) != SQLITE_OK) {
		write_error("Failed to open database!\n");
		sqlite3_close(db);
		return false;
	}

	char *errmsg = NULL;
	sqlite3_exec(db, "PRAGMA locking_mode = EXCLUSIVE;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		write_error("Failed to open database in exclusive locking mode: %s!\n", errmsg);
		goto error;
	}

	// All dates are stored as the number of seconds since 1970.
	// Note that numeric arguments in parentheses that following the type name
	// are ignored by SQLite - there's no need to impose any length limits.
	//
	// feeds' download_date and update_date are special timestamps:
	// download_date stores the date of last successful feed download,
	// update_date stores the date of last feed download attempt.
	sqlite3_exec(
		db,
		"CREATE TABLE IF NOT EXISTS feeds("
			"feed_url TEXT NOT NULL UNIQUE,"
			"title TEXT,"
			"link TEXT,"
			"content TEXT,"
			"attachments TEXT,"
			"persons TEXT,"
			"extras TEXT,"
			"download_date INTEGER NOT NULL DEFAULT 0,"
			"update_date INTEGER NOT NULL DEFAULT 0,"
			"time_to_live INTEGER NOT NULL DEFAULT 0,"
			"http_header_etag TEXT,"
			"http_header_last_modified INTEGER NOT NULL DEFAULT 0,"
			"http_header_expires INTEGER NOT NULL DEFAULT 0"
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed_url TEXT NOT NULL,"
			"guid TEXT,"
			"title TEXT,"
			"link TEXT,"
			"content TEXT,"
			"attachments TEXT,"
			"persons TEXT,"
			"extras TEXT,"
			"publication_date INTEGER NOT NULL DEFAULT 0,"
			"update_date INTEGER NOT NULL DEFAULT 0,"
			"unread INTEGER NOT NULL DEFAULT 0,"
			"important INTEGER NOT NULL DEFAULT 0"
		");"
		"CREATE INDEX IF NOT EXISTS idx_items_guid ON items("
			"feed_url,"
			"guid"
		");"
		"CREATE INDEX IF NOT EXISTS idx_items_essentials ON items("
			"feed_url,"
			"title,"
			"link,"
			"publication_date,"
			"update_date,"
			"unread,"
			"important"
		");",
		NULL,
		NULL,
		&errmsg
	);
	if (errmsg != NULL) {
		write_error("Failed to initialize database in exclusive locking mode: %s!\n", errmsg);
		goto error;
	}

	return true;
error:
	write_error("Probably there's other process currently working with this database.\n");
	sqlite3_free(errmsg);
	sqlite3_close(db);
	return false;
}

bool
db_vacuum(void)
{
	char *error = NULL;
	sqlite3_exec(db, "VACUUM;", NULL, NULL, &error);
	if (error != NULL) {
		write_error("Failed to vacuum the database: %s!\n", error);
		sqlite3_free(error);
		return false;
	}
	return true;
}

bool
exec_database_file_optimization(void)
{
	if (get_cfg_bool(NULL, CFG_CLEAN_DATABASE_ON_STARTUP) == true) {
		if (db_vacuum() == false) {
			return false;
		}
	}
	char *error = NULL;
	if (get_cfg_bool(NULL, CFG_ANALYZE_DATABASE_ON_STARTUP) == true) {
		sqlite3_exec(db, "ANALYZE;", NULL, NULL, &error);
		if (error != NULL) {
			write_error("Failed to analyze the database: %s!\n", error);
			sqlite3_free(error);
			return false;
		}
	}
	return true;
}

bool
db_begin_transaction(void)
{
	INFO("Starting database transaction.");
	char *errmsg;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not begin transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

bool
db_commit_transaction(void)
{
	INFO("Committing database transaction.");
	char *errmsg;
	sqlite3_exec(db, "COMMIT;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not commit transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

bool
db_rollback_transaction(void)
{
	INFO("Rolling back database transaction.");
	char *errmsg;
	sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not rollback transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

void
db_stop(void)
{
	sqlite3_close(db);
}

sqlite3_stmt *
db_prepare(const char *statement, int size, const char **p_error)
{
	sqlite3_stmt *stmt = NULL;
	// Last argument to sqlite3_prepare_v2() is a pointer to unused portion of
	// statement (sqlite3_prepare_v2() will prepare only first command from the
	// statement). But given that we always pass one command in the statement,
	// this parameter is practically useless.
	int status = sqlite3_prepare_v2(db, statement, size, &stmt, NULL);
	if (status != SQLITE_OK) {
		// We don't use sqlite3_errmsg() here because it's not thread-safe.
		const char *error = sqlite3_errstr(status);
		error = error ? error : "unknown error";
		FAIL("Failed to prepare statement, %s: %s", error, statement);
		if (p_error) {
			*p_error = error;
		}
		return NULL;
	}
	INFO("Prepared statement: %s", statement);
	return stmt;
}

const char *
db_error_string(void)
{
	// FIXME: sqlite3_errmsg() is not thread-safe
	return sqlite3_errmsg(db);
}

int
db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str)
{
	if (STRING_IS_EMPTY(str)) {
		return sqlite3_bind_null(stmt, pos);
	} else {
		return sqlite3_bind_text(stmt, pos, str->ptr, str->len, SQLITE_STATIC);
	}
}

int64_t
db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len)
{
	char query[100] = {0}; // longest column name (25) + rest of query (35) + terminator (1) < 100
	memcpy(query, "SELECT ", 7);
	memcpy(query + 7, column, column_len);
	memcpy(query + 7 + column_len, " FROM feeds WHERE feed_url=?", 29);
	sqlite3_stmt *res = db_prepare(query, 7 + column_len + 29, NULL);
	if (res == NULL) {
		return -1;
	}
	db_bind_string(res, 1, url);
	int64_t date = sqlite3_step(res) == SQLITE_ROW ? sqlite3_column_int64(res, 0) : 0;
	sqlite3_finalize(res);
	INFO("%s of %s is %" PRId64, column, url->ptr, date);
	return date;
}

struct string *
db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len)
{
	char query[100] = {0};
	memcpy(query, "SELECT ", 7);
	memcpy(query + 7, column, column_len);
	memcpy(query + 7 + column_len, " FROM feeds WHERE feed_url=?", 29);
	sqlite3_stmt *res = db_prepare(query, 7 + column_len + 29, NULL);
	if (res != NULL) {
		db_bind_string(res, 1, url);
		if (sqlite3_step(res) == SQLITE_ROW) {
			const char *str_ptr = (char *)sqlite3_column_text(res, 0);
			if (str_ptr != NULL) {
				size_t str_len = strlen(str_ptr);
				if (str_len > 0) {
					struct string *str = crtas(str_ptr, str_len);
					sqlite3_finalize(res);
					return str;
				}
			}
		}
		sqlite3_finalize(res);
	}
	return NULL;
}

void
db_update_feed_int64(const struct string *url, const char *column_name, int64_t value, bool only_positive)
{
	if (only_positive == false || value > 0) {
		char query[100] = {0};
		strcpy(query, "UPDATE feeds SET ");
		strcat(query, column_name);
		strcat(query, "=? WHERE feed_url=?");
		INFO("Setting %s of %s to %" PRId64, column_name, url->ptr, value);
		sqlite3_stmt *res = db_prepare(query, strlen(query) + 1, NULL);
		if (res != NULL) {
			sqlite3_bind_int64(res, 1, value);
			db_bind_string(res, 2, url);
			if (sqlite3_step(res) != SQLITE_DONE) {
				FAIL("Failed to update %s!", column_name);
			}
			sqlite3_finalize(res);
		}
	}
}

void
db_update_feed_string(const struct string *url, const char *column_name, const struct string *value, bool only_nonempty)
{
	if (only_nonempty == false || (value != NULL && value->len > 0)) {
		char query[100] = {0};
		strcpy(query, "UPDATE feeds SET ");
		strcat(query, column_name);
		strcat(query, "=? WHERE feed_url=?");
		INFO("Setting %s of %s to %s", column_name, url->ptr, value->ptr);
		sqlite3_stmt *res = db_prepare(query, strlen(query) + 1, NULL);
		if (res != NULL) {
			db_bind_string(res, 1, value);
			db_bind_string(res, 2, url);
			if (sqlite3_step(res) != SQLITE_DONE) {
				FAIL("Failed to update %s!", column_name);
			}
			sqlite3_finalize(res);
		}
	}
}
