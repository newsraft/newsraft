#include <string.h>
#include "newsraft.h"

static inline void
remove_trailing_slashes_from_string(struct string *str)
{
	while ((str->len > 0) && (str->ptr[str->len - 1] == '/')) {
		str->len -= 1;
	}
	str->ptr[str->len] = '\0';
}

static inline bool
check_url_for_validity(const struct string *url)
{
	if (strncmp(url->ptr, "http://", 7) != 0
		&& strncmp(url->ptr, "https://", 8) != 0
		&& strncmp(url->ptr, "ftp://", 6) != 0
		&& strncmp(url->ptr, "file://", 7) != 0
		&& strncmp(url->ptr, "gopher://", 9) != 0
		&& strncmp(url->ptr, "gophers://", 10) != 0)
	{
		fprintf(stderr, "Stumbled across an invalid URL: \"%s\"!\n", url->ptr);
		fputs("Every feed URL must start with a protocol scheme like \"http://\".\n", stderr);
		fputs("Supported protocol schemes are http, https, ftp, file, gopher and gophers.\n", stderr);
		return false;
	}
	return true;
}

bool
parse_feeds_file(void)
{
	const char *feeds_file_path = get_feeds_path();
	if (feeds_file_path == NULL) {
		return false;
	}
	if (make_sure_section_exists(get_cfg_string(CFG_GLOBAL_SECTION_NAME)) != 0) {
		return false;
	}
	FILE *f = fopen(feeds_file_path, "r");
	if (f == NULL) {
		fputs("Couldn't open feeds file!\n", stderr);
		return false;
	}
	struct string *line = crtes(1000);
	bool at_least_one_feed_was_added = false;
	int64_t section_index = 0;
	int64_t global_update_period = -1;
	int64_t section_update_period = -1;
	struct string *section_name = crtes(100);
	struct feed_entry feed;
	feed.name = crtes(100);
	feed.link = crtes(200);
	if (section_name == NULL || feed.name == NULL || feed.link == NULL) {
		fputs("Not enough memory for parsing feeds file!\n", stderr);
		goto error;
	}

	int c = '@';
	while (c != EOF) {

		empty_string(line);
		for (c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
			catcs(line, c);
		}
		trim_whitespace_from_string(line);

		if (line->len == 0 || line->ptr[0] == '#') continue; // Skip empty and comment lines.

		// Negative value indicates that the timer should be inherited from the parent or set to zero.
		long update_timer = -1;
		if (line->ptr[line->len - 1] == ']' || line->ptr[line->len - 1] == '}') {
			size_t close_pos = line->len - 1;
			size_t start_pos = close_pos;
			while (start_pos > 0 && line->ptr[start_pos - 1] != '[' && line->ptr[start_pos - 1] != '{') {
				start_pos -= 1;
			}
			if (start_pos == 0) {
				fputs("Update timer closing bracket is specified without an opening bracket!\n", stderr);
				goto error;
			}
			if (sscanf(line->ptr + start_pos, "%ld", &update_timer) != 1 || update_timer < 0) {
				fputs("Update timer set to the invalid value!\n", stderr);
				goto error;
			}
			update_timer *= 60; // Convert to seconds.
			if (start_pos < 2) {
				fputs("Stumbled upon an invalid feed line!\n", stderr);
				goto error;
			}
			line->len = start_pos - 2;
			line->ptr[line->len] = '\0';
			trim_whitespace_from_string(line);
		}

		if (line->ptr[0] == '@') { // Start of a new section.
			cpyas(&section_name, line->ptr + 1, line->len - 1);
			trim_whitespace_from_string(section_name);
			section_update_period = update_timer > 0 ? update_timer : global_update_period;
			section_index = make_sure_section_exists(section_name);
			if (section_index < 0) goto error;
			if (section_index == 0) {
				global_update_period = section_update_period;
			}
			continue;
		}

		empty_string(feed.name);
		if (line->ptr[line->len - 1] == '"') {
			size_t close_pos = line->len - 1;
			size_t start_pos = close_pos;
			while (start_pos > 0 && line->ptr[start_pos - 1] != '"') {
				start_pos -= 1;
			}
			if (start_pos < 2) {
				fputs("Stumbled upon an invalid feed line!\n", stderr);
				goto error;
			}
			line->ptr[close_pos] = '\0';
			cpyas(&feed.name, line->ptr + start_pos, strlen(line->ptr + start_pos));
			line->len = start_pos - 2;
			line->ptr[line->len] = '\0';
			trim_whitespace_from_string(line);
		}

		cpyss(&feed.link, line);
		feed.update_period = update_timer > 0 ? update_timer : section_update_period;
		remove_trailing_slashes_from_string(feed.link);
		if (line->len < 4 || line->ptr[0] != '$' || line->ptr[1] != '(' || line->ptr[line->len - 1] != ')') {
			if (check_url_for_validity(feed.link) == false) {
				goto error;
			}
		}

		if (copy_feed_to_section(&feed, section_index) == true) {
			at_least_one_feed_was_added = true;
		} else {
			fprintf(stderr, "Failed to add feed \"%s\" to section \"%s\"!\n", feed.link->ptr, section_name->ptr);
			goto error;
		}

	}

	if (at_least_one_feed_was_added == false) {
		fputs("Not a single feed was loaded!\n", stderr);
		goto error;
	}

	free_string(line);
	free_string(section_name);
	free_string(feed.link);
	free_string(feed.name);
	fclose(f);
	return true;
error:
	free_string(line);
	free_string(section_name);
	free_string(feed.link);
	free_string(feed.name);
	fclose(f);
	return false;
}
