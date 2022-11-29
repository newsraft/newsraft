#include <string.h>
#include "newsraft.h"

static sqlite3 *db;
static pthread_t database_file_optimization_thread;

bool
db_init(void)
{
	const char *path = get_db_path();
	if (path == NULL) {
		// Error message written by get_db_path.
		return false;
	}

	if (sqlite3_config(SQLITE_CONFIG_SERIALIZED) != SQLITE_OK) {
		fputs("Failed to set threading mode of database to serialized!\n", stderr);
		fputs("It probably happens because SQLite was compiled without multithreading functionality.\n", stderr);
		return false;
	}

	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fputs("Failed to open database!\n", stderr);
		sqlite3_close(db);
		return false;
	}

	char *errmsg;
	// All dates are stored as the number of seconds since 1970.
	// Note that numeric arguments in parentheses that following the type name
	// are ignored by SQLite - there's no need to impose any length limits.
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
		fprintf(stderr, "Failed to initialize database: %s!\n", errmsg);
		sqlite3_close(db);
		sqlite3_free(errmsg);
		return false;
	}

	return true;
}

static void *
database_file_optimization_routine(void *dummy)
{
	(void)dummy;
	char *errmsg = NULL;
	if (get_cfg_bool(CFG_CLEAN_DATABASE_ON_STARTUP) == true) {
		sqlite3_exec(db, "VACUUM;", NULL, NULL, &errmsg);
	}
	if (get_cfg_bool(CFG_ANALYZE_DATABASE_ON_STARTUP) == true) {
		sqlite3_exec(db, "ANALYZE;", NULL, NULL, &errmsg);
	}
	return errmsg;
}

// Note to the future.
// This should be done in the background (separate thread), because VACUUM and
// ANALYZE database queries take very long time in case of big database files.
bool
start_database_file_optimization(void)
{
	if (pthread_create(&database_file_optimization_thread, NULL, &database_file_optimization_routine, NULL) != 0) {
		fputs("Failed to create database optimization thread!\n", stderr);
		return false;
	}
	return true;
}

bool
catch_database_file_optimization(void)
{
	char *errmsg = NULL;
	if (pthread_join(database_file_optimization_thread, (void **)&errmsg) != 0) {
		fputs("Failed to finish database file optimization!\n", stderr);
		return false;
	}
	if (errmsg != NULL) {
		fprintf(stderr, "Failed to optimize the database: %s!\n", errmsg);
		sqlite3_free(errmsg);
		return false;
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
db_prepare(const char *zSql, int nByte)
{
	sqlite3_stmt *pStmt;
	// Last argument to sqlite3_prepare_v2 is pointer to unused portion of zSql
	// (sqlite3_prepare_v2 will prepare only first statement). But given that we
	// always pass one statement to the function, this parameter is useless.
	if (sqlite3_prepare_v2(db, zSql, nByte, &pStmt, NULL) != SQLITE_OK) {
		FAIL("Failed to prepare \"%s\" statement: %s", zSql, sqlite3_errmsg(db));
		return NULL;
	}
	INFO("Prepared \"%s\" statement.", zSql);
	return pStmt;
}

const char *
db_error_string(void)
{
	return sqlite3_errmsg(db);
}

int
db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str)
{
	if ((str == NULL) || (str->len == 0)) {
		return sqlite3_bind_null(stmt, pos);
	} else {
		return sqlite3_bind_text(stmt, pos, str->ptr, str->len, NULL);
	}
}

int64_t
db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len)
{
	char query[100]; // Longest column name (25) + rest of query (36) + terminator (1) = 100 x)
	memcpy(query, "SELECT ", 7);
	memcpy(query + 7, column, column_len);
	memcpy(query + 7 + column_len, " FROM feeds WHERE feed_url=?", 29);
	sqlite3_stmt *res = db_prepare(query, 7 + column_len + 29);
	if (res == NULL) {
		return -1;
	}
	sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
	int64_t date;
	if (sqlite3_step(res) == SQLITE_ROW) {
		date = sqlite3_column_int64(res, 0);
	} else {
		date = 0; // This is the first reloading of the feed.
	}
	sqlite3_finalize(res);
	return date;
}

struct string *
db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len)
{
	char query[100];
	memcpy(query, "SELECT ", 7);
	memcpy(query + 7, column, column_len);
	memcpy(query + 7 + column_len, " FROM feeds WHERE feed_url=?", 29);
	sqlite3_stmt *res = db_prepare(query, 7 + column_len + 29);
	if (res != NULL) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		if (sqlite3_step(res) == SQLITE_ROW) {
			const char *str_ptr = (char *)sqlite3_column_text(res, 0);
			if (str_ptr != NULL) {
				const size_t str_len = strlen(str_ptr);
				if (str_len != 0) {
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
