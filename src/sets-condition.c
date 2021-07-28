#include "feedeater.h"

static struct set_condition *
create_set_condition_for_feed(struct string *feed_url)
{
	struct set_condition *st = malloc(sizeof(struct set_condition));
	if (st == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for feed set_condition!\n");
		return NULL;
	}
	if ((st->db_cmd = create_string(" feed = ?", 9)) == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for WHERE condition of set_condition!\n");
		free(st);
		return NULL;
	}
	if ((st->urls = malloc(sizeof(struct string *))) == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for urls of set_condition!\n");
		free_string(st->db_cmd);
		free(st);
		return NULL;
	}
	st->urls[0] = feed_url;
	st->urls_count = 1;
	return st;
}

static struct set_condition *
create_set_condition_for_filter(struct string *tags_expr)
{
	struct set_condition *st = malloc(sizeof(struct set_condition));
	if (st == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for feed set_condition!\n");
		return NULL;
	}
	if ((st->db_cmd = create_empty_string()) == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for WHERE condition of set_condition!\n");
		free(st);
		return NULL;
	}
	st->urls = NULL;
	st->urls_count = 0;
	char word[1000], c;
	size_t word_len = 0, i = 0, old_urls_count;
	struct feed_tag *tag;
	while (1) {
		c = tags_expr->ptr[i++];
		if (c == '&' || c == '|' || c == ')' || c == '\0') {
			if (word_len != 0) {
				word[word_len] = '\0';
				word_len = 0;
				tag = get_tag_by_name(word);
				if (tag == NULL) {
					free_string(st->db_cmd);
					free(st->urls);
					free(st);
					status_write("[error] tag \"%s\" does not exist!", word);
					return NULL;
				}
				old_urls_count = st->urls_count;
				st->urls_count += tag->urls_count;
				st->urls = realloc(st->urls, sizeof(struct string *) * st->urls_count);
				cat_string_array(st->db_cmd, " (", 2);
				for (size_t j = 0; j < tag->urls_count; ++j) {
					cat_string_array(st->db_cmd, " feed = ?", 9);
					st->urls[old_urls_count++] = tag->urls[j];
					if (j + 1 != tag->urls_count) cat_string_array(st->db_cmd, " OR", 3);
				}
				cat_string_array(st->db_cmd, " )", 2);
			}
			if (c == '&') {
				cat_string_array(st->db_cmd, " AND", 4);
			} else if (c == '|') {
				cat_string_array(st->db_cmd, " OR", 3);
			} else if (c == ')') {
				cat_string_array(st->db_cmd, " )", 2);
			} else if (c == '\0') {
				break;
			}
		} else if (c == '(') {
			cat_string_array(st->db_cmd, " (", 2);
		} else {
			word[word_len++] = c;
		}
	}
	return st;
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
