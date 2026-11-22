#include <string.h>
#include <unistd.h>
#include "newsraft.h"

static const char expected_content[] =
	"\x1f^"
	"\x1ftype=xhtml"
	"\x1ftext="
	"<div xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\" class=\"h-entry\">\n"
	"\t\t\t\t<p>Some shell code:</p>\n"
	"\t\t\t\t<pre><code>for i in `seq 0 10`; do\n"
	"\techo $((i * i))\n"
	"done</code></pre>"
	"</div>";

bool
newsraft_test(void)
{
	const char *feed_url = "$(cat ./tests/assets/atom-xhtml-conversion-test.xml)";

	int fds[2];
	NEWSRAFT_ASSERT(pipe(fds) == 0);
	NEWSRAFT_ASSERT(write(fds[1], feed_url, strlen(feed_url)) == (ssize_t)strlen(feed_url));
	close(fds[1]);
	NEWSRAFT_ASSERT(dup2(fds[0], STDIN_FILENO) == STDIN_FILENO);
	close(fds[0]);

	set_feeds_path("/dev/stdin");

	NEWSRAFT_ASSERT(db_init());
	NEWSRAFT_ASSERT(parse_feeds_file());
	NEWSRAFT_ASSERT(curl_init());
	NEWSRAFT_ASSERT(threads_start());
	start_updating_all_feeds_and_wait_finish();
	they_want_us_to_stop = true;
	threads_stop();
	curl_stop();

	const char *query = "SELECT content FROM items WHERE guid='https://example.org/shell-programming'";
	sqlite3_stmt *res = db_prepare(query, strlen(query) + 1, NULL);
	NEWSRAFT_ASSERT(res);

	bool status = true;
	NEWSRAFT_ASSERT(sqlite3_step(res) == SQLITE_ROW);
	const char *content = (const char *)sqlite3_column_text(res, 0);
	NEWSRAFT_ASSERT(content);
	if (strcmp(content, expected_content) != 0) {
		fprintf(stderr, "Expected content:\n%s\n\nActual content:\n%s\n", expected_content, content);
		status = false;
	}
	sqlite3_finalize(res);

	free_sections();
	db_stop();

	return status;
}
