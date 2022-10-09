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
check_url_for_validity(const struct string *str)
{
	if (str->len == 0) {
		fputs("Feed URL is empty!\n", stderr);
		return false;
	}
	if (strstr(str->ptr, "://") == NULL) {
		fputs("Feed URL is missing protocol scheme!\n", stderr);
		fputs("Every feed URL must start with a protocol definition like \"http://\".\n", stderr);
		return false;
	}
	if ((strncmp(str->ptr, "http://", 7) != 0)
		&& (strncmp(str->ptr, "https://", 8) != 0)
		&& (strncmp(str->ptr, "ftp://", 6) != 0)
		&& (strncmp(str->ptr, "file://", 7) != 0))
	{
		fputs("Feed URL has unknown protocol scheme!\n", stderr);
		fputs("Supported protocols are http, https, ftp and file.\n", stderr);
		return false;
	}
	return true;
}

bool
parse_feeds_file(void)
{
	const char *feeds_file_path = get_feeds_path();
	if (feeds_file_path == NULL) {
		// Error message written by get_feeds_path.
		return false;
	}
	FILE *f = fopen(feeds_file_path, "r");
	if (f == NULL) {
		fputs("Couldn't open feeds file!\n", stderr);
		return false;
	}
	const struct string *global_section_name = get_cfg_string(CFG_GLOBAL_SECTION_NAME);
	struct string *section_name = crtss(global_section_name);
	if (section_name == NULL) {
		fclose(f);
		return false;
	}
	struct feed_line feed;
	feed.name = crtes();
	if (feed.name == NULL) {
		free_string(section_name);
		fclose(f);
		return false;
	}
	feed.link = crtes();
	if (feed.link == NULL) {
		free_string(feed.name);
		free_string(section_name);
		fclose(f);
		return false;
	}
	size_t feeds_count = 0;
	struct string *tmp;

	char c;
	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (true) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (ISWHITESPACE(c));

		if (c == '#') {
			// Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == EOF) {
				break;
			}
			continue;
		} else if (c == '@') {
			empty_string(section_name);
			do { c = fgetc(f); } while (ISWHITESPACEEXCEPTNEWLINE(c));
			while ((c != '\n') && (c != EOF)) {
				if (catcs(section_name, c) == false) { goto error; }
				c = fgetc(f);
			}
			if (c == EOF) {
				break;
			}
			continue;
		} else if (c == EOF) {
			break;
		}

		empty_string(feed.name);
		empty_string(feed.link);

		while (true) {
			if (catcs(feed.link, c) == false) { goto error; }
			c = fgetc(f);
			if (ISWHITESPACE(c) || c == EOF) { break; }
		}
		remove_trailing_slashes_from_string(feed.link);
		if (check_url_for_validity(feed.link) == false) {
			fprintf(stderr, "Stumbled across an invalid URL: \"%s\"!\n", feed.link->ptr);
			goto error;
		}
		while (ISWHITESPACEEXCEPTNEWLINE(c)) { c = fgetc(f); }
		// process name
		if (c == '"') {
			while (true) {
				c = fgetc(f);
				if (c == '"' || c == '\n' || c == EOF) { break; }
				if (catcs(feed.name, c) == false) { goto error; }
			}
		}
		if (feed.name->len == 0) {
			tmp = db_get_string_from_feed_table(feed.link, "title", 5);
			if (tmp != NULL) {
				inlinefy_string(tmp);
				cpyss(feed.name, tmp);
				free_string(tmp);
			}
		}
		if (copy_feed_to_section(&feed, section_name) == true) {
			feeds_count += 1;
		} else {
			fprintf(stderr, "Failed to add feed \"%s\" to section \"%s\"!\n", feed.link->ptr, section_name->ptr);
			goto error;
		}

		// Skip everything to the next newline character.
		if (c != '\n') {
			if (c == EOF) {
				break;
			}
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			if (c == EOF) {
				break;
			}
		}

	}

	if (feeds_count == 0) {
		fputs("Not a single feed was loaded!\n", stderr);
		goto error;
	}

	free_string(feed.link);
	free_string(feed.name);
	free_string(section_name);
	fclose(f);
	return true;
error:
	free_sections();
	free_string(feed.link);
	free_string(feed.name);
	free_string(section_name);
	fclose(f);
	return false;
}
