#include <stdlib.h>
#include "feedeater.h"

bool
append_empty_string_to_string_list(struct string_list **list)
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

void
free_string_list(struct string_list *list)
{
	struct string_list *item = list;
	struct string_list *temp;
	while (item != NULL) {
		temp = item;
		item = item->next;
		free_string(temp->str);
		free(temp);
	}
}
