#include <stdlib.h>
#include "newsraft.h"

static const char separators[] = {',', ';', '|', '/', '\\', '-', '~', '*', '@', '&', ':', '+', '=', '!', '%', '_', '.', '\0'};

bool
prepend_empty_string_to_string_list(struct string_list **list)
{
	struct string_list *new = malloc(sizeof(struct string_list));
	if (new == NULL) {
		return false;
	}
	new->str = crtes();
	if (new->str == NULL) {
		free(new);
		return false;
	}
	new->next = *list;
	*list = new;
	return true;
}

bool
copy_string_to_string_list(struct string_list **list, const struct string *src)
{
	struct string_list *new = malloc(sizeof(struct string_list));
	if (new == NULL) {
		return false;
	}
	new->str = crtss(src);
	if (new->str == NULL) {
		free(new);
		return false;
	}
	new->next = *list;
	*list = new;
	return true;
}

struct string *
concatenate_strings_of_string_list_into_one_string(const struct string_list *list)
{
	if (list == NULL) {
		return crtes();
	}
	const struct string_list *l;
	char separator = ',';
	bool list_has_conflicting_char;
	for (const char *i = separators; *i != '\0'; ++i) {
		list_has_conflicting_char = false;
		l = list;
		while (l != NULL) {
			if (strchr(l->str->ptr, *i) != NULL) {
				list_has_conflicting_char = true;
				break;
			}
			l = l->next;
		}
		if (list_has_conflicting_char == false) {
			separator = *i;
			break;
		}
	}
	l = list;
	struct string *result = crtss(l->str);
	if (result == NULL) {
		return NULL;
	}
	while (true) {
		l = l->next;
		if (l == NULL) {
			break;
		}
		if (l->str->len != 0) {
			if ((separator != ',') && (separator != ';')) {
				catcs(result, ' ');
			}
			catcs(result, separator);
			catcs(result, ' ');
			catss(result, l->str);
		}
	}
	return result;
}

void
reverse_string_list(struct string_list **list)
{
	struct string_list *prev = NULL;
	struct string_list *current = *list;
	struct string_list *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
}

void
free_string_list(struct string_list *list)
{
	struct string_list *l = list;
	struct string_list *temp;
	while (l != NULL) {
		temp = l;
		l = l->next;
		free_string(temp->str);
		free(temp);
	}
}
