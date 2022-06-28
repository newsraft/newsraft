#include <stdio.h>
#include "newsraft.h"

bool
parse_feeds_file(const char *path)
{
	FILE *f = fopen(path, "r");
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
				cpyss(feed.name, tmp);
				free_string(tmp);
			}
		}
		if (copy_feed_to_section(&feed, section_name) == false) {
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
