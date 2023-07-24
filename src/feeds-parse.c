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
	if ((strncmp(url->ptr, "http://", 7) != 0)
		&& (strncmp(url->ptr, "https://", 8) != 0)
		&& (strncmp(url->ptr, "ftp://", 6) != 0)
		&& (strncmp(url->ptr, "gopher://", 9) != 0)
		&& (strncmp(url->ptr, "file://", 7) != 0))
	{
		fprintf(stderr, "Stumbled across an invalid URL: \"%s\"!\n", url->ptr);
		fputs("Every feed URL must start with a protocol scheme like \"http://\".\n", stderr);
		fputs("Supported protocol schemes are http, https, ftp, gopher and file.\n", stderr);
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
	bool at_least_one_feed_was_added = false;
	int64_t section_index = 0;
	int64_t global_update_period = -1;
	int64_t section_update_period = -1;
	struct string *section_name = crtes(100);
#define UPDATE_TIME_SIZE 17 // Allow less than 19 characters to not overflow int64_t.
	char update_time[UPDATE_TIME_SIZE + 1];
	size_t update_time_len;
	struct feed_entry feed;
	feed.name = crtes(100);
	feed.link = crtes(200);
	if ((section_name == NULL) || (feed.name == NULL) || (feed.link == NULL)) {
		fputs("Not enough memory for parsing feeds file!\n", stderr);
		goto error;
	}

	int c;
	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (true) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (ISWHITESPACE(c));

		if (c == '#') { // Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			continue;
		} else if (c == '@') { // Start a new section.
			empty_string(section_name);
			for (c = fgetc(f); (c != '{') && (c != '\n') && (c != EOF); c = fgetc(f)) {
				if (catcs(section_name, c) == false) { goto error; }
			}
			section_update_period = global_update_period;
			if (c == '{') {
				update_time_len = 0;
				for (c = fgetc(f); (update_time_len < UPDATE_TIME_SIZE) && (c != '}') && (c != '\n') && (c != EOF); c = fgetc(f)) {
					update_time[update_time_len++] = c;
				}
				update_time[update_time_len] = '\0';
				if (sscanf(update_time, "%" SCNd64, &section_update_period) != 1) {
					fputs("Encountered an invalid section update period value!\n", stderr);
					goto error;
				}
				section_update_period *= 60; // Convert to seconds.
				while ((c != '\n') && (c != EOF)) { c = fgetc(f); }
			}
			trim_whitespace_from_string(section_name);
			section_index = make_sure_section_exists(section_name);
			if (section_index == 0) {
				global_update_period = section_update_period;
			}
			if (section_index < 0) { goto error; }
			continue;
		} else if (c == EOF) {
			break;
		}

		empty_string(feed.name);
		empty_string(feed.link);
		feed.update_period = section_update_period;

		while (true) {
			if (catcs(feed.link, c) == false) { goto error; }
			c = fgetc(f);
			if (ISWHITESPACE(c) || c == EOF) { break; }
		}
		if (check_url_for_validity(feed.link) == false) {
			goto error;
		}
		remove_trailing_slashes_from_string(feed.link);
		while (ISWHITESPACEEXCEPTNEWLINE(c)) { c = fgetc(f); }
		// process name
		if (c == '"') {
			while (true) {
				c = fgetc(f);
				if (c == '"' || c == '\n' || c == EOF) { break; }
				if (catcs(feed.name, c) == false) { goto error; }
			}
		}
		while ((c != '{') && (c != '\n') && (c != EOF)) { c = fgetc(f); }
		if (c == '{') {
			update_time_len = 0;
			for (c = fgetc(f); (update_time_len < UPDATE_TIME_SIZE) && (c != '}') && (c != '\n') && (c != EOF); c = fgetc(f)) {
				update_time[update_time_len++] = c;
			}
			update_time[update_time_len] = '\0';
			if (sscanf(update_time, "%" SCNd64, &feed.update_period) != 1) {
				fputs("Encountered an invalid feed update period value!\n", stderr);
				goto error;
			}
			feed.update_period *= 60; // Convert to seconds.
		}
		if (copy_feed_to_section(&feed, section_index) == true) {
			at_least_one_feed_was_added = true;
		} else {
			fprintf(stderr, "Failed to add feed \"%s\" to section \"%s\"!\n", feed.link->ptr, section_name->ptr);
			goto error;
		}

		while ((c != '\n') && (c != EOF)) { c = fgetc(f); }
	}

	if (at_least_one_feed_was_added == false) {
		fputs("Not a single feed was loaded!\n", stderr);
		goto error;
	}

	free_string(section_name);
	free_string(feed.link);
	free_string(feed.name);
	fclose(f);
	return true;
error:
	free_string(section_name);
	free_string(feed.link);
	free_string(feed.name);
	fclose(f);
	return false;
}
