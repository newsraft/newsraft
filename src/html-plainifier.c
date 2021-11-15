#include <string.h>
#include "feedeater.h"

#define INIT_ATT_SIZE 30

enum html_status {
	HTML_DEFAULT = 0,
	HTML_ORDERED_LIST = 1,
	HTML_UNORDERED_LIST = 2,
	HTML_PREFORMATTED = 4,
};

static char **atts = NULL;                     // array of attributes of current html tag
static size_t atts_count = 0;                  // number of attributes in current html tag
static uint8_t list_depth;
static uint8_t lists_length[10];
static enum html_status status;

static void
free_atts(void)
{
	if (atts == NULL) {
		return;
	}
	for (size_t i = 0; i < atts_count; ++i) {
		free(atts[i]);
	}
	atts_count = 0;
	free(atts);
	atts = NULL;
}

#define TAG_IS_BLOCK(A) (strcmp(A, "div")  == 0 || \
                         strcmp(A, "/div") == 0 || \
                         strcmp(A, "p")    == 0 || \
                         strcmp(A, "/p")   == 0 || \
                         strcmp(A, "ol")   == 0 || \
                         strcmp(A, "/ol")  == 0 || \
                         strcmp(A, "ul")   == 0 || \
                         strcmp(A, "/ul")  == 0 || \
                         strcmp(A, "dl")   == 0 || \
                         strcmp(A, "/dl")  == 0 || \
                         strcmp(A, "dt")   == 0 || \
                         strcmp(A, "/dt")  == 0 || \
                         strcmp(A, "dd")   == 0 || \
                         strcmp(A, "/dd")  == 0 || \
                         strcmp(A, "pre")  == 0 || \
                         strcmp(A, "/pre") == 0)
#define TAG_IS_HEADER_START(A) (strcmp(A, "h1") == 0 || \
                                strcmp(A, "h2") == 0 || \
                                strcmp(A, "h3") == 0 || \
                                strcmp(A, "h4") == 0 || \
                                strcmp(A, "h5") == 0 || \
                                strcmp(A, "h6") == 0 || \
                                strcmp(A, "footer") == 0)
#define TAG_IS_HEADER_END(A) (strcmp(A, "/h1") == 0 || \
                              strcmp(A, "/h2") == 0 || \
                              strcmp(A, "/h3") == 0 || \
                              strcmp(A, "/h4") == 0 || \
                              strcmp(A, "/h5") == 0 || \
                              strcmp(A, "/h6") == 0 || \
                              strcmp(A, "/footer") == 0)

/* Append some characters according to the current HTML attributes. */
static void
format_text(char *text, size_t *iter)
{
	if (strcmp(atts[0], "ol") == 0) {
		status |= HTML_ORDERED_LIST;
		++list_depth;
		lists_length[list_depth - 1] = 0;
	} else if (strcmp(atts[0], "/ol") == 0) {
		status &= HTML_ORDERED_LIST;
		--list_depth;
	} else if (strcmp(atts[0], "ul") == 0) {
		status |= HTML_UNORDERED_LIST;
		++list_depth;
	} else if (strcmp(atts[0], "/ul") == 0) {
		status &= HTML_UNORDERED_LIST;
		--list_depth;
	} else if (strcmp(atts[0], "pre") == 0) {
		status |= HTML_PREFORMATTED;
	} else if (strcmp(atts[0], "/pre") == 0) {
		status &= ~HTML_PREFORMATTED;
	}

	if (*iter == 0) {
		return;
	}

	if (TAG_IS_BLOCK(atts[0]) || TAG_IS_HEADER_END(atts[0])) {
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
	} else if (strcmp(atts[0], "li") == 0) {
		if (text[*iter - 1] != '\n') {
			text[(*iter)++] = '\n';
		}
		text[(*iter)++] = ' ';
		for (uint8_t i = 1; i < list_depth; ++i) {
			text[(*iter)++] = ' ';
			text[(*iter)++] = ' ';
			text[(*iter)++] = ' ';
			text[(*iter)++] = ' ';
		}
		if ((status & HTML_ORDERED_LIST) != 0) {
			++lists_length[list_depth - 1];
			char *number_str = malloc(sizeof(char) * 10);
			if (number_str != NULL) {
				snprintf(number_str, 10, "%d. ", lists_length[list_depth - 1]);
				text[(*iter)] = '\0';
				strcat(text, number_str);
				(*iter) += strlen(number_str);
				free(number_str);
			}
		} else {
			text[(*iter)++] = '*';
			text[(*iter)++] = ' ';
			text[(*iter)++] = ' ';
		}
	} else if (strcmp(atts[0], "br") == 0) {
		text[(*iter)++] = '\n';
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

	status = HTML_DEFAULT;
	list_depth = 0;
	size_t j = 0,          /* plain text iterator */
	       att_index,      /* index of last attribute of html tag */
	       att_limit,      /* maximum length of current attribute */
	       att_char;       /* actual length of current attribute */
	bool   in_tag = false; /* shows if buf->ptr[i] character belongs to html tag */
	bool   error = false;  /* shows if error occurred in loop */

	// parse html text char-by-char
	for (size_t i = 0; i < buf->len; ++i) {
		if (in_tag == true) {
			if (buf->ptr[i] == '>') {
				in_tag = false;
				if (atts != NULL) {
					atts[att_index][att_char] = '\0';
					format_text(text, &j);
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
					if (atts == NULL) {
						error = true;
						break;
					}
					att_limit = INIT_ATT_SIZE;
					atts[att_index] = malloc(sizeof(char) * att_limit);
					if (atts[att_index] == NULL) {
						error = true;
						break;
					}
				} else if (att_char + 1 >= att_limit) {
					att_limit = (att_char + 1) * 2;
					atts[att_index] = realloc(atts[att_index], sizeof(char) * att_limit);
					if (atts[att_index] == NULL) {
						error = true;
						break;
					}
				}
				atts[att_index][att_char++] = buf->ptr[i];
			}
		} else {
			if (buf->ptr[i] == '<') {
				in_tag = true;
				free_atts();
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

	free_string(buf);
	free_atts();

	if (error == true) {
		free(text);
		return NULL;
	}

	struct string *text_buf = malloc(sizeof(struct string));
	if (text_buf == NULL) {
		free(text);
		return NULL;
	}
	text[j] = '\0';
	text_buf->ptr = text;
	text_buf->len = j;

	strip_whitespace_from_edges(text_buf->ptr, &(text_buf->len));

	return text_buf;
}
