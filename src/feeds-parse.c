#include <string.h>
#include "newsraft.h"

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
		write_error("Stumbled across an invalid URL: \"%s\"!\n", url->ptr);
		write_error("Every feed URL must start with a protocol scheme like \"http://\".\n");
		write_error("Supported protocol schemes are http, https, ftp, file, gopher and gophers.\n");
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
	if (make_sure_section_exists(get_cfg_string(NULL, CFG_GLOBAL_SECTION_NAME)) != 0) {
		return false;
	}
	FILE *f = fopen(feeds_file_path, "r");
	if (f == NULL) {
		write_error("Couldn't open feeds file!\n");
		return false;
	}
	bool status = false;
	int64_t section_index       = 0;
	struct string *line         = crtes(200);
	struct string *section_name = crtes(200);
	struct string *feed_cfg     = crtes(200);
	struct string *section_cfg  = crtes(200);
	struct string *global_cfg   = crtes(200);
	bool at_least_one_feed_was_added = false;
	struct feed_entry feed = {
		.url  = crtes(200),
		.name = crtes(100),
	};

	int c = '@';
	while (c != EOF) {

		empty_string(line);
		for (c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
			catcs(line, c);
		}
		trim_whitespace_from_string(line);

		if (line->len == 0 || line->ptr[0] == '#') continue; // Skip empty and comment lines.

		bool is_section = false;

		// Process first token: section, feed command or feed link
		size_t len = 0;
		if (line->ptr[0] == '@') {
			is_section = true;
			for (const char *i = line->ptr + 1; *i != '[' && *i != '{' && *i != '<' && *i != '\0'; ++i) {
				len += 1;
			}
			cpyas(&section_name, line->ptr + 1, len);
			trim_whitespace_from_string(section_name);
			section_index = make_sure_section_exists(section_name);
			if (section_index < 0) goto error;
			empty_string(section_cfg);
			remove_start_of_string(line, 1 + len);
		} else if (line->ptr[0] == '$' && line->ptr[1] == '(') {
			for (len = 2; line->ptr[len] != '\0'; ++len) {
				if (line->ptr[len] == ')') {
					len += 1;
					break;
				}
			}
			cpyas(&feed.url, line->ptr, len);
			remove_start_of_string(line, len);
		} else {
			for (const char *i = line->ptr; !ISWHITESPACE(*i) && *i != '\0'; ++i) {
				len += 1;
			}
			cpyas(&feed.url, line->ptr, len);
			// Here you may want to remove the trailing slashes at the end of the URL.
			// We MUST NOT do that, because a URL with and without a trailing slash can
			// point to different resources. Treating them alike breaks HTTP semantics.
			// Actually removing the trailing slashes backfired at us once. Here's link:
			// https://codeberg.org/newsraft/newsraft/issues/224
			if (!check_url_for_validity(feed.url)) {
				goto error;
			}
			remove_start_of_string(line, len);
		}
		trim_whitespace_from_string(line);
		empty_string(feed_cfg);

		// Process second token: feed name
		empty_string(feed.name);
		if (line->ptr[0] == '"') {
			char *close = strchr(line->ptr + 1, '"');
			if (close == NULL) {
				write_error("Unclosed feed name!\n");
				goto error;
			}
			*close = '\0';
			cpyas(&feed.name, line->ptr + 1, strlen(line->ptr + 1));
			remove_start_of_string(line, close + 1 - line->ptr);
		}
		trim_whitespace_from_string(line);

		if (line->ptr[0] == '[' || line->ptr[0] == '{') {
			write_error("Counters in [square] and {curly} brackets are deprecated!\n");
			write_error("You must set reload-period and item-limit settings to individual feeds and sections instead.\n");
			write_error("For example:\n");
			write_error("http://example.org/feed.xml < reload-period 30; item-limit 100\n");
			goto error;
		}

		if (line->ptr[0] == '<') {
			catas(is_section ? section_cfg : feed_cfg, line->ptr + 1, strlen(line->ptr + 1));
		}
		if (is_section == true && section_index == 0) {
			catss(global_cfg, section_cfg);
		}

		if (is_section == true)
			continue;

		struct feed_entry *feed_ptr = copy_feed_to_section(&feed, section_index);
		if (feed_ptr == NULL)
			goto error;

		at_least_one_feed_was_added = true;

		if (!process_config_line(feed_ptr, global_cfg->ptr,  global_cfg->len))  goto error;
		if (!process_config_line(feed_ptr, section_cfg->ptr, section_cfg->len)) goto error;
		if (!process_config_line(feed_ptr, feed_cfg->ptr,    feed_cfg->len))    goto error;

		// Count unread items only after config is applied because some settings
		// can influence how items are counted, for example item-rule setting.
		feed_ptr->unread_count = db_count_items(&feed_ptr, 1, true);
		feed_ptr->items_count  = db_count_items(&feed_ptr, 1, false);
	}

	if (at_least_one_feed_was_added == false) {
		write_error("Not a single feed was loaded!\n");
		goto error;
	}

	status = true;
error:
	free_string(line);
	free_string(section_name);
	free_string(feed_cfg);
	free_string(section_cfg);
	free_string(global_cfg);
	free_string(feed.url);
	free_string(feed.name);
	fclose(f);
	return status;
}
