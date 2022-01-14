#include <stdlib.h>
#include "feedeater.h"

// Note to the future.
// When disastrous failure (like shortage of memory) occurrs in
// "create_set_condition_for_feed" or "create_set_condition_for_multi_feed",
// print failure messages to stderr, because at the moment of creation of
// set conditions there is no curses interface, and on errors of such kind
// application is supposed to exit as soon as possible. Hence it is better
// to display error message just in front of disappointed user, rather than
// silently writing them to log file leaving the user confused (moreover,
// in most cases the log file is not even written).

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

// On success returns pointer to set_condition struct.
// On failure returns NULL.
const struct set_condition *
create_set_condition_for_feed(const struct string *feed_url)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		fprintf(stderr, "Not enough memory for feed set condition!\n");
		return NULL;
	}
	if ((sc->db_cmd = crtas("(feed_url=?1)", 13)) == NULL) {
		fprintf(stderr, "Not enough memory for WHERE condition of feed set condition!\n");
		free(sc);
		return NULL;
	}
	if ((sc->urls = malloc(sizeof(struct string *))) == NULL) {
		fprintf(stderr, "Not enough memory for urls list of feed set condition!\n");
		free_string(sc->db_cmd);
		free(sc);
		return NULL;
	}
	sc->urls[0] = feed_url;
	sc->urls_count = 1;
	return sc;
}

// On success returns 0.
// On failure returns non-zero:
// 	on shortage of memory returns 1.
static inline int
append_urls_of_tag_to_set_condition(const struct feed_tag *head_tag, struct set_condition *sc, const char *tag_name)
{
	const struct feed_tag *tag = get_tag_by_name(head_tag, tag_name);

	if (tag == NULL) {
		// There is no tags under that name, so just append FALSE to WHERE condition.
		if (catcs(sc->db_cmd, '0') == false) {
			return 1;
		}
		return 0;
	}

	// tag always has at least one url
	//if (tag->urls_count == 0) {
	//	// Lu Lu Lu, I've got some apples, Lu Lu Lu, you got some too...
	//}

	if (catcs(sc->db_cmd, '(') == false) {
		return 1;
	}

	bool found_this_url_in_sc;
	size_t index_of_url_in_sc;
	char word[100];
	size_t word_len;
	const struct string **temp;

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
			word_len = sprintf(word, "feed_url=?%lu", index_of_url_in_sc + 1);
		} else {
			temp = realloc(sc->urls, sizeof(struct string *) * (sc->urls_count + 1));
			if (temp != NULL) {
				sc->urls = temp;
			} else {
				return 1;
			}
			++(sc->urls_count);
			sc->urls[sc->urls_count - 1] = tag->urls[i];
			word_len = sprintf(word, "feed_url=?%lu", sc->urls_count);
		}

		if (catas(sc->db_cmd, word, word_len) == false) {
			return 1;
		}
		if ((i + 1) != tag->urls_count) {
			if (catas(sc->db_cmd, " OR ", 4) == false) {
				return 1;
			}
		}

	}

	if (catcs(sc->db_cmd, ')') == false) {
		return 1;
	}

	return 0;
}

// On success returns pointer to set_condition struct.
// On failure returns NULL.
const struct set_condition *
create_set_condition_for_multi_feed(const struct feed_tag *head_tag, const struct string *tags_expr)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		fprintf(stderr, "Not enough memory for multi-feed set condition!\n");
		return NULL;
	}
	if ((sc->db_cmd = crtas("(", 1)) == NULL) {
		fprintf(stderr, "Not enough memory for WHERE condition of multi-feed set condition!\n");
		free(sc);
		return NULL;
	}
	sc->urls = NULL;
	sc->urls_count = 0;

	char c;
	int error = 0;
	size_t word_len = 0;
	size_t i = 0;
	size_t word_lim = INIT_WORD_BUFF_SIZE;
	char *temp;

	char *word = malloc(sizeof(char) * word_lim); // buffer for tags' names
	if (word == NULL) {
		fprintf(stderr, "Not enough memory for word thing to create set condition!");
		free_set_condition(sc);
		return NULL;
	}

	while (1) {
		c = tags_expr->ptr[i++];
		if (ISWHITESPACE(c)) {
			continue;
		} else if (c == '&' || c == '|' || c == ')' || c == '\0') {
			if (word_len != 0) {
				word[word_len] = '\0';
				word_len = 0;
				if (append_urls_of_tag_to_set_condition(head_tag, sc, word) != 0) {
					error = 1;
					break;
				}
			}
			if (c == '&') {
				if (catas(sc->db_cmd, " AND ", 5) == false) {
					error = 1;
					break;
				}
			} else if (c == '|') {
				if (catas(sc->db_cmd, " OR ", 4) == false) {
					error = 1;
					break;
				}
			} else if (c == ')') {
				if (catcs(sc->db_cmd, ')') == false) {
					error = 1;
					break;
				}
			} else if (c == '\0') {
				break;
			}
		} else if (c == '(') {
			if (word_len == 0) {
				if (catcs(sc->db_cmd, '(') == false) {
					error = 1;
					break;
				}
			} else {
				error = 2;
				break;
			}
		} else {
			if (word_len == (word_lim - 1)) {
				word_lim = word_lim * 2;
				temp = realloc(word, sizeof(char) * word_lim);
				if (temp != NULL) {
					word = temp;
				} else {
					error = 1;
					break;
				}
			}
			word[word_len++] = c;
		}
	}

	free(word);

	if (error != 0) {
		if (error == 1) {
			fprintf(stderr, "Not enough memory for multi-feed set condition!\n");
		} else if (error == 2) {
			fprintf(stderr, "Bad place for \"(\" in \"%s\"!\n", tags_expr->ptr);
		}
		free_set_condition(sc);
		return NULL;
	}

	if (catcs(sc->db_cmd, ')') == false) {
		fprintf(stderr, "Not enough memory for multi-feed set condition!\n");
		free_set_condition(sc);
		return NULL;
	}

	return sc;
}
