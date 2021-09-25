#include <string.h>
#include "feedeater.h"

#define INIT_ATT_SIZE 30

enum html_status {
	HTML_DEFAULT = 0,
	HTML_PREFORMATTED = 1,
};

static void
free_atts(char ***atts, size_t atts_count)
{
	if (*atts == NULL) {
		return;
	}
	for (size_t i = 0; i < atts_count; ++i) {
		free((*atts)[i]);
	}
	free(*atts);
	*atts = NULL;
}

#define TAG_IS_BLOCK(A) (strcmp(A, "div") == 0    || \
                         strcmp(A, "/div") == 0   || \
                         strcmp(A, "ol") == 0     || \
                         strcmp(A, "/ol") == 0    || \
                         strcmp(A, "ul") == 0     || \
                         strcmp(A, "/ul") == 0    || \
                         strcmp(A, "li") == 0     || \
                         strcmp(A, "/li") == 0    || \
                         strcmp(A, "dl") == 0     || \
                         strcmp(A, "/dl") == 0    || \
                         strcmp(A, "dt") == 0     || \
                         strcmp(A, "/dt") == 0    || \
                         strcmp(A, "dd") == 0     || \
                         strcmp(A, "/dd") == 0    || \
                         strcmp(A, "pre") == 0    || \
                         strcmp(A, "/pre") == 0   || \
                         strcmp(A, "footer") == 0 || \
                         strcmp(A, "/footer") == 0)
#define TAG_IS_HEADER_START(A) (strcmp(A, "h1") == 0 || \
                                strcmp(A, "h2") == 0 || \
                                strcmp(A, "h3") == 0 || \
                                strcmp(A, "h4") == 0 || \
                                strcmp(A, "h5") == 0 || \
                                strcmp(A, "h6") == 0)
#define TAG_IS_HEADER_END(A) (strcmp(A, "/h1") == 0 || \
                              strcmp(A, "/h2") == 0 || \
                              strcmp(A, "/h3") == 0 || \
                              strcmp(A, "/h4") == 0 || \
                              strcmp(A, "/h5") == 0 || \
                              strcmp(A, "/h6") == 0)
#define TAG_IS_PARAGRAPH(A) (strcmp(A, "p") == 0  || \
                             strcmp(A, "/p") == 0)

/*
change plain text according to current html attributes

text - plain text string
iter - plain text iterator
*/
static void
format_plain_text(char **atts, char *text, size_t *iter)
{
	if (*iter == 0) {
		return;
	}
	if (TAG_IS_PARAGRAPH(atts[0]) || TAG_IS_HEADER_END(atts[0])) {
		if (text[*iter - 1] == '\n') {
			if (*iter >= 2 && text[*iter - 2] == '\n') {
				// enough
			} else {
				text[(*iter)++] = '\n';
			}
		} else {
			text[(*iter)++] = '\n';
			text[(*iter)++] = '\n';
		}
	} else if (TAG_IS_HEADER_START(atts[0])) {
		if (text[*iter - 1] == '\n') {
			if (*iter >= 2 && text[*iter - 2] == '\n') {
				if (*iter >= 3 && text[*iter - 3] == '\n') {
					// enough
				} else {
					text[(*iter)++] = '\n';
				}
			} else {
				text[(*iter)++] = '\n';
				text[(*iter)++] = '\n';
			}
		} else {
			text[(*iter)++] = '\n';
			text[(*iter)++] = '\n';
			text[(*iter)++] = '\n';
		}
	} else if (TAG_IS_BLOCK(atts[0])) {
		if (text[*iter - 1] != '\n') {
			text[(*iter)++] = '\n';
		}
	} else if (strcmp(atts[0], "br") == 0) {
		text[(*iter)++] = '\n';
	}
}

static void
update_html_status(char **atts, enum html_status *status_ptr)
{
	if (strcmp(atts[0], "pre") == 0) {
		*status_ptr |= HTML_PREFORMATTED;
	} else if (strcmp(atts[0], "/pre") == 0) {
		*status_ptr &= ~HTML_PREFORMATTED;
	}

}

struct string *
plainify_html(char *buf_with_entities, size_t buf_len)
{
	struct string *buf = expand_html_entities(buf_with_entities, buf_len);
	if (buf == NULL) {
		return NULL;
	}
	char *text = malloc(sizeof(char) * (buf->len + 1));
	if (text == NULL) {
		free_string(buf);
		return NULL;
	}

	char   **atts = NULL;  /* array of html tag attributes */
	size_t i,              /* html text iterator */
	       j,              /* plain text iterator */
	       atts_count = 0, /* number of attribute elements in atts */
	       att_index,      /* index of last attribute of html tag */
	       att_limit,      /* maximum length of current attribute */
	       att_char;       /* actual length of current attribute */
	bool   in_tag = false; /* shows if buf->ptr[i] character belongs to html tag */
	enum html_status status = HTML_DEFAULT;

	// parse html text char-by-char
	for (i = 0, j = 0; i < buf->len; ++i) {
		if (in_tag == true) {
			if (buf->ptr[i] == '>') {
				in_tag = false;
				if (atts != NULL) {
					atts[att_index][att_char] = '\0';
					update_html_status(atts, &status);
					format_plain_text(atts, text, &j);
				}
			} else if (buf->ptr[i] == ' ' || buf->ptr[i] == '\n') {
				if (atts != NULL) {
					atts[att_index][att_char] = '\0';
					att_char = 0;
				}
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
				atts[att_index][att_char++] = buf->ptr[i];
			}
		} else {
			if (buf->ptr[i] == '<') {
				in_tag = true;
				free_atts(&atts, atts_count);
				atts_count = 0;
				att_char = 0;
			} else if ((status & HTML_PREFORMATTED) == 0 && (buf->ptr[i] == ' ' || buf->ptr[i] == '\n' || buf->ptr[i] == '\t')) {
				if (j != 0 && text[j - 1] != ' ' && text[j - 1] != '\n' && text[j - 1] != '\t') {
					text[j++] = ' ';
				}
			} else {
				text[j++] = buf->ptr[i];
			}
		}
	}

	text[j] = '\0';

	free_string(buf);
	free_atts(&atts, atts_count);

	struct string *text_buf = malloc(sizeof(struct string));
	if (text_buf == NULL) {
		free(text);
		return NULL;
	}
	text_buf->ptr = text;
	text_buf->len = j;

	strip_whitespace_from_edges(text_buf->ptr, &(text_buf->len));

	return text_buf;
}
