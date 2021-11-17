#include "feedeater.h"

#define INIT_WORD_BUFF_SIZE 50 // do not set it to 0

void
free_set_condition(struct set_condition *sc)
{
	free(sc->urls);
	free_string(sc->db_cmd);
	free(sc);
}

static struct set_condition *
create_set_condition_for_feed(struct string *feed_url)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		debug_write(DBG_FAIL, "Can not allocate memory for feed set_condition!\n");
		return NULL;
	}
	if ((sc->db_cmd = create_string(" feed = ?", 9)) == NULL) {
		debug_write(DBG_FAIL, "Can not allocate memory for WHERE condition of set_condition!\n");
		free(sc);
		return NULL;
	}
	if ((sc->urls = malloc(sizeof(struct string *))) == NULL) {
		debug_write(DBG_FAIL, "Can not allocate memory for urls of set_condition!\n");
		free_string(sc->db_cmd);
		free(sc);
		return NULL;
	}
	sc->urls[0] = feed_url;
	sc->urls_count = 1;
	return sc;
}

static struct set_condition *
create_set_condition_for_filter(struct string *tags_expr)
{
	struct set_condition *sc = malloc(sizeof(struct set_condition));
	if (sc == NULL) {
		debug_write(DBG_FAIL, "Can not allocate memory for feed set_condition!\n");
		return NULL;
	}
	if ((sc->db_cmd = create_empty_string()) == NULL) {
		debug_write(DBG_FAIL, "Can not allocate memory for WHERE condition of set_condition!\n");
		free(sc);
		return NULL;
	}
	sc->urls = NULL;
	sc->urls_count = 0;
	char c;
	size_t word_len = 0, i = 0, old_urls_count;
	size_t word_lim = INIT_WORD_BUFF_SIZE;
	struct feed_tag *tag;
	char *word = malloc(sizeof(char) * word_lim); // buffer for tags' names
	if (word == NULL) {
		free_set_condition(sc);
		debug_write(DBG_FAIL, "Can not allocate memory for word thing to create set condition!\n");
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
					free_set_condition(sc);
					status_write("[error] tag \"%s\" does not exist!", word);
					return NULL;
				}
				old_urls_count = sc->urls_count;
				sc->urls_count += tag->urls_count;
				sc->urls = realloc(sc->urls, sizeof(struct string *) * sc->urls_count);
				cat_string_array(sc->db_cmd, " (", 2);
				for (size_t j = 0; j < tag->urls_count; ++j) {
					cat_string_array(sc->db_cmd, " feed = ?", 9);
					sc->urls[old_urls_count++] = tag->urls[j];
					if (j + 1 != tag->urls_count) cat_string_array(sc->db_cmd, " OR", 3);
				}
				cat_string_array(sc->db_cmd, " )", 2);
			}
			if (c == '&') {
				cat_string_array(sc->db_cmd, " AND", 4);
			} else if (c == '|') {
				cat_string_array(sc->db_cmd, " OR", 3);
			} else if (c == ')') {
				cat_string_array(sc->db_cmd, " )", 2);
			} else if (c == '\0') {
				break;
			}
		} else if (c == '(') {
			if (word_len == 0) {
				cat_string_array(sc->db_cmd, " (", 2);
			} else {
				free_set_condition(sc);
				status_write("[error] bad syntax", word);
				return NULL;
			}
		} else {
			if (word_len == (word_lim - 1)) {
				word_lim = word_lim * 2;
				word = realloc(word, sizeof(char) * word_lim);
				if (word == NULL) {
					free_set_condition(sc);
					status_write("[error] not enough memory");
					return NULL;
				}
			}
			word[word_len++] = c;
		}
	}
	free(word);
	return sc;
}

struct set_condition *
create_set_condition(struct set_line *set)
{
	if (set->link != NULL) {
		return create_set_condition_for_feed(set->link);
	} else if (set->tags != NULL) {
		return create_set_condition_for_filter(set->tags);
	}
	return NULL;
}
