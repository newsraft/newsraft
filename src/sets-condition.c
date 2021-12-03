#include <stdlib.h>
#include "feedeater.h"

#define INIT_WORD_BUFF_SIZE 50 // do not set it to 0

void
free_set_condition(const struct set_condition *sc)
{
	if (sc == NULL) {
		return;
	}
	free(sc->urls);
	free_string(sc->db_cmd);
	free((void *)sc);
}

const struct set_condition *
create_set_condition_for_feed(const struct string *feed_url)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		FAIL("Not enough memory for feed set condition!");
		return NULL; // failure
	}
	if ((sc->db_cmd = create_string("(feed=?1)", 9)) == NULL) {
		FAIL("Not enough memory for WHERE condition of feed set condition!");
		free(sc);
		return NULL; // failure
	}
	if ((sc->urls = malloc(sizeof(struct string *))) == NULL) {
		FAIL("Not enough memory for urls list of feed set condition!");
		free_string(sc->db_cmd);
		free(sc);
		return NULL; // failure
	}
	sc->urls[0] = feed_url;
	sc->urls_count = 1;
	return sc; // success
}

static int
append_urls_of_tag_to_set_condition(struct set_condition *sc, const struct feed_tag *tag)
{
	if (tag->urls_count == 0) {
		FAIL("Tag is empty!");
		return 1; // failure
	}

	bool found_this_url_in_sc;
	size_t index_of_url_in_sc;
	char word[100];
	size_t word_len;

	catcs(sc->db_cmd, '(');

	for (size_t i = 0; i < tag->urls_count; ++i) {

		found_this_url_in_sc = false;
		for (size_t j = 0; j < sc->urls_count; ++j) {
			if (tag->urls[i] == sc->urls[j]) {
				found_this_url_in_sc = true;
				index_of_url_in_sc = j;
				break;
			}
		}

		if (found_this_url_in_sc == true) {
			word_len = sprintf(word, "feed=?%lu", index_of_url_in_sc + 1);
		} else {
			++(sc->urls_count);
			sc->urls = realloc(sc->urls, sizeof(struct string *) * sc->urls_count);
			sc->urls[sc->urls_count - 1] = tag->urls[i];
			word_len = sprintf(word, "feed=?%lu", sc->urls_count);
		}

		catas(sc->db_cmd, word, word_len);
		if ((i + 1) != tag->urls_count) catas(sc->db_cmd, " OR ", 4);

	}

	catcs(sc->db_cmd, ')');

	return 0; // success
}


const struct set_condition *
create_set_condition_for_filter(const struct string *tags_expr)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		FAIL("Not enough memory for filter set condition!");
		return NULL; // failure
	}
	if ((sc->db_cmd = create_string("(", 1)) == NULL) {
		FAIL("Not enough memory for WHERE condition of filter set condition!");
		free(sc);
		return NULL; // failure
	}
	sc->urls = NULL;
	sc->urls_count = 0;

	char c;
	bool error = false;
	size_t word_len = 0, i = 0;
	size_t word_lim = INIT_WORD_BUFF_SIZE;
	const struct feed_tag *tag;

	char *word = malloc(sizeof(char) * word_lim); // buffer for tags' names
	if (word == NULL) {
		free_set_condition(sc);
		FAIL("Not enough memory for word thing to create set condition!");
		return NULL;
	}

	while (1) {
		c = tags_expr->ptr[i++];
		if (c == '&' || c == '|' || c == ')' || c == '\0') {
			if (word_len != 0) {
				word[word_len] = '\0';
				word_len = 0;
				tag = get_tag_by_name(word);
				if (tag == NULL) {
					error = true;
					break;
				}
				if (append_urls_of_tag_to_set_condition(sc, tag) != 0) {
					error = true;
					break;
				}
			}
			if (c == '&') {
				catas(sc->db_cmd, " AND ", 5);
			} else if (c == '|') {
				catas(sc->db_cmd, " OR ", 4);
			} else if (c == ')') {
				catcs(sc->db_cmd, ')');
			} else if (c == '\0') {
				break;
			}
		} else if (c == '(') {
			if (word_len == 0) {
				catcs(sc->db_cmd, '(');
			} else {
				error = true;
				break;
			}
		} else {
			if (word_len == (word_lim - 1)) {
				word_lim = word_lim * 2;
				word = realloc(word, sizeof(char) * word_lim);
				if (word == NULL) {
					error = true;
					break;
				}
			}
			word[word_len++] = c;
		}
	}

	free(word);

	if (error == true) {
		free_set_condition(sc);
		return NULL; // failure
	}

	catcs(sc->db_cmd, ')');

	return sc; // success
}
