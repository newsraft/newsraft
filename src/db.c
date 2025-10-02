#include <string.h>
#include <regex.h>
#include "newsraft.h"

static sqlite3 *db;

static void
newsraft_regexp_cmp(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	if (argc != 2) {
		sqlite3_result_error(ctx, "regexp() called with invalid number of arguments.\n", -1);
		return;
	}

	const char *exp = (const char *)sqlite3_value_text(argv[0]);
	const char *text = (const char *)sqlite3_value_text(argv[1]);
	if (exp == NULL || text == NULL) {
		sqlite3_result_error(ctx, "regexp() called with invalid argument values.\n", -1);
		return;
	}

	regex_t regex;
	if (regcomp(&regex, exp, REG_EXTENDED | REG_NOSUB) != 0) {
		sqlite3_result_error(ctx, "regexp() failed to compile regular expression.\n", -1);
		return;
	}

	int ret = regexec(&regex, text , 0, NULL, 0);
	regfree(&regex);
	sqlite3_result_int(ctx, ret != REG_NOMATCH);
}

static bool
db_make_sure_column_exists(const char *table, const char *column, const char *type)
{
	char query[100];
	int len = snprintf(query, sizeof(query), "PRAGMA table_info(%s);", table);
	if (len <= 0 || len >= (int)sizeof(query)) {
		return false;
	}
	sqlite3_stmt *stmt = db_prepare(query, len + 1, NULL);
	if (stmt == NULL) {
		write_error("Failed to query table_info from the database: %s!\n", sqlite3_errmsg(db));
		return false;
	}
	bool column_exists = false;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		if (strcmp(name, column) == 0) {
			column_exists = true;
			break;
		}
	}
	sqlite3_finalize(stmt);
	if (column_exists) {
		INFO("Column %s (%s) already exists in %s table", column, type, table);
		return true;
	}
	len = snprintf(query, sizeof(query), "ALTER TABLE %s ADD COLUMN %s %s;", table, column, type);
	if (len <= 0 || len >= (int)sizeof(query)) {
		return false;
	}
	char *errmsg = NULL;
	if (sqlite3_exec(db, query, NULL, NULL, &errmsg) != SQLITE_OK || errmsg != NULL) {
		write_error("Failed to add %s column to the %s table in database: %s!\n", column, table, errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

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

	if (sqlite3_create_function(db, "regexp", 2, SQLITE_ANY, 0, &newsraft_regexp_cmp, 0, 0) != SQLITE_OK) {
		write_error("Failed to register REGEXP database function!\n");
		goto error;
	}

	// All dates are stored as the number of seconds since 1970.
	// Note that numeric arguments in parentheses that follow the type name
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
			"http_header_expires INTEGER NOT NULL DEFAULT 0,"
			"user_data TEXT"
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed_url TEXT NOT NULL,"
			"guid TEXT NOT NULL,"
			"title TEXT,"
			"link TEXT,"
			"content TEXT,"
			"attachments TEXT,"
			"persons TEXT,"
			"extras TEXT,"
			"download_date INTEGER NOT NULL DEFAULT 0,"
			"publication_date INTEGER NOT NULL DEFAULT 0,"
			"update_date INTEGER NOT NULL DEFAULT 0,"
			"unread INTEGER NOT NULL DEFAULT 0,"
			"important INTEGER NOT NULL DEFAULT 0,"
			"user_data TEXT"
		");"
		"CREATE INDEX IF NOT EXISTS idx_items_guid ON items("
			"feed_url,"
			"guid"
		");"
		"CREATE INDEX IF NOT EXISTS idx_items_eight_way ON items("
			"feed_url,"
			"guid,"
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

	// These columns were implemented after users already created their database files,
	// therefore we have to make sure that existing database tables get them post factum.
	if (!db_make_sure_column_exists("feeds", "user_data", "TEXT")) {
		goto error;
	}
	if (!db_make_sure_column_exists("items", "download_date", "INTEGER NOT NULL DEFAULT 0")) {
		goto error;
	}
	if (!db_make_sure_column_exists("items", "user_data", "TEXT")) {
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
	if (get_cfg_bool(NULL, CFG_DATABASE_CLEAN_ON_STARTUP) == true) {
		if (db_vacuum() == false) {
			return false;
		}
	}
	char *error = NULL;
	if (get_cfg_bool(NULL, CFG_DATABASE_ANALYZE_ON_STARTUP) == true) {
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
db_transaction_begin(void)
{
	if (!get_cfg_bool(NULL, CFG_DATABASE_BATCH_TRANSACTIONS)) {
		INFO("Not beginning database transaction because it's turned off.");
		return true;
	}
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
db_transaction_commit(void)
{
	if (!get_cfg_bool(NULL, CFG_DATABASE_BATCH_TRANSACTIONS)) {
		INFO("Not committing database transaction because it's turned off.");
		return true;
	}
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

// You might wonder why we would ever need a special function to bind a feed URL
// specifically. Well, that's simple: sometimes user specifies URL with trailing
// slashes, sometimes without them. These URLs can point to different content in
// practice, but for the most part it's either
// * both URLs (with and without trailing slashes) point to the same content, or
// * one URL points to useful content and the other one is simply empty (404ing)
//
// Database-wise, we should treat these URLs as a single entity,
// so that the user doesn't have to suffer from the fragmentation of the
// same feed based on the presence or absence of slashes in the URL.
int
db_bind_feed_url(sqlite3_stmt *stmt, int pos, const struct string *str)
{
	if (STRING_IS_EMPTY(str)) {
		return sqlite3_bind_null(stmt, pos);
	}
	size_t len = str->len;
	while (len > 0 && str->ptr[len - 1] == '/') {
		len -= 1;
	}
	return sqlite3_bind_text(stmt, pos, str->ptr, len, SQLITE_STATIC);
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
	db_bind_feed_url(res, 1, url);
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
		db_bind_feed_url(res, 1, url);
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
			db_bind_feed_url(res, 2, url);
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
			db_bind_feed_url(res, 2, url);
			if (sqlite3_step(res) != SQLITE_DONE) {
				FAIL("Failed to update %s!", column_name);
			}
			sqlite3_finalize(res);
		}
	}
}

void
db_perform_user_edit(const struct wstring *fmt, struct feed_entry **feeds, size_t feeds_count, const struct item_entry *item)
{
	struct timespec start = newsraft_get_monotonic_time();
	size_t replacements_count = 0;
	struct wstring *cmd = wcrtes(1000);
	for (size_t i = 0; i + 8 < fmt->len; ++i) {
		if (wcsncmp(fmt->ptr + i, L"@selected", 9) == 0) {
			i += 8;
			if (item) {
				wcatas(cmd, L"(rowid=? AND feed_url=? AND guid=?)", 35);
				replacements_count += 1;
				continue;
			}
			if (feeds && feeds_count > 0) {
				wcatcs(cmd, L'(');
				for (size_t i = 0; i < feeds_count; ++i) {
					if (i > 0) {
						wcatas(cmd, L" OR ", 4);
					}
					wcatas(cmd, L"feed_url=?", 10);
				}
				wcatcs(cmd, L')');
				replacements_count += 1;
				continue;
			}
		} else {
			wcatcs(cmd, fmt->ptr[i]);
		}
	}
	struct string *query = convert_wstring_to_string(cmd);
	free_wstring(cmd);
	if (query == NULL) {
		return;
	}
	const char *error = "";
	sqlite3_stmt *stmt = db_prepare(query->ptr, query->len + 1, &error);
	if (stmt == NULL) {
		fail_status("%s: %ls", error, fmt->ptr);
		free_string(query);
		return;
	}
	for (size_t i = 0, j = 1; i < replacements_count; ++i) {
		if (item) {
			sqlite3_bind_int64(stmt, j++, item->rowid);
			db_bind_feed_url(stmt, j++, item->feed[0]->url);
			db_bind_string(stmt, j++, item->guid);
			continue;
		}
		if (feeds && feeds_count > 0) {
			for (size_t k = 0; k < feeds_count; ++k) {
				db_bind_feed_url(stmt, j++, feeds[k]->url);
			}
			continue;
		}
	}
	while (true) {
		int status = sqlite3_step(stmt);
		if (status == SQLITE_DONE) {
			int changes = sqlite3_changes(db);
			struct timespec stop = newsraft_get_monotonic_time();
			struct string *time_diff = newsraft_get_pretty_time_diff(&start, &stop);
			info_status("Successful edit (%d change%s took %s)", changes, changes > 1 ? "s" : "", time_diff->ptr);
			free_string(time_diff);
			break;
		}
		if (status != SQLITE_ROW) {
			fail_status("Failed: %ls", fmt->ptr);
			break;
		}
	}
	sqlite3_finalize(stmt);
	free_string(query);
}
