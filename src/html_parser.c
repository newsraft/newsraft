#include <string.h>
#include "feedeater.h"

#define INIT_ATT_SIZE 30

static char **atts = NULL;
static size_t atts_count = 0;

static void
free_atts(void)
{
	if (atts != NULL) {
		for (size_t a = 0; a < atts_count; ++a) {
			free(atts[a]);
		}
		free(atts);
		atts = NULL; // set to NULL because we will call realloc on atts later
	}
	atts_count = 0;
}

static int
tag_is_block_elem(char *tag)
{
	return strcmp(tag, "div") == 0 ||
	       strcmp(tag, "/div") == 0 ||
	       strcmp(tag, "ol") == 0 ||
	       strcmp(tag, "/ol") == 0 ||
	       strcmp(tag, "ul") == 0 ||
	       strcmp(tag, "/ul") == 0 ||
	       strcmp(tag, "li") == 0 ||
	       strcmp(tag, "/li") == 0 ||
	       strcmp(tag, "dl") == 0 ||
	       strcmp(tag, "/dl") == 0 ||
	       strcmp(tag, "dt") == 0 ||
	       strcmp(tag, "/dt") == 0 ||
	       strcmp(tag, "dd") == 0 ||
	       strcmp(tag, "/dd") == 0 ||
	       strcmp(tag, "footer") == 0 ||
	       strcmp(tag, "/footer") == 0;
}

#define TAG_IS_HEADER(A) strcmp(A, "h1") == 0 || \
                         strcmp(A, "/h1") == 0 || \
                         strcmp(A, "h2") == 0 || \
                         strcmp(A, "/h2") == 0 || \
                         strcmp(A, "h3") == 0 || \
                         strcmp(A, "/h3") == 0 || \
                         strcmp(A, "h4") == 0 || \
                         strcmp(A, "/h4") == 0 || \
                         strcmp(A, "h5") == 0 || \
                         strcmp(A, "/h5") == 0 || \
                         strcmp(A, "h6") == 0 || \
                         strcmp(A, "/h6") == 0
#define TAG_IS_PARAGRAPH(A) strcmp(A, "p") == 0 || \
                            strcmp(A, "/p") == 0

static void
format_plain_text(char *text, size_t *iter)
{
	if (*iter == 0) {
		return;
	}
	if (TAG_IS_HEADER(atts[0])) {
		if (text[*iter - 1] != '\n') {
			text[(*iter)++] = '\n';
		}
		text[(*iter)++] = '\n';
	} else if (TAG_IS_PARAGRAPH(atts[0])) {
		text[(*iter)++] = '\n';
	} else if (tag_is_block_elem(atts[0])) {
		if (text[*iter - 1] != '\n') {
			text[(*iter)++] = '\n';
		}
	} else if (strcmp(atts[0], "br") == 0) {
		text[(*iter)++] = '\n';
	}
}

char *
plainify_html(char *buff, size_t buff_len)
{
	char *text = malloc(sizeof(char) * (buff_len + 1));
	if (text == NULL) {
		return NULL;
	}
	bool in_tag = false;
	size_t i, j, att_index, att_limit, att_char;
	for (i = 0, j = 0; i < buff_len; ++i) {
		if (in_tag == true) {
			if (buff[i] == '>') {
				in_tag = false;
				atts[att_index][att_char] = '\0';
				format_plain_text(text, &j);
			} else if (buff[i] == ' ' || buff[i] == '\n') {
				atts[att_index][att_char] = '\0';
				att_char = 0;
			} else {
				if (att_char == 0) {
					att_index = atts_count++;
					atts = realloc(atts, sizeof(char *) * atts_count);
					att_limit = INIT_ATT_SIZE;
					atts[att_index] = malloc(sizeof(char) * att_limit);
				} else if (att_char + 1 >= att_limit) {
					att_limit = (att_char + 1) * 2;
					atts[att_index] = realloc(atts[att_index], sizeof(char) * att_limit);
				}
				atts[att_index][att_char++] = buff[i];
			}
		} else {
			if (buff[i] == '<') {
				in_tag = true;
				free_atts();
				att_char = 0;
			} else if (buff[i] == '\n' || buff[i] == ' ') {
				if (j != 0 && text[j - 1] != ' ' && text[j - 1] != '\n') {
					text[j++] = ' ';
				}
			} else {
				text[j++] = buff[i];
			}
		}
	}
	text[j] = '\0';
	free_atts();
	return text;
}
