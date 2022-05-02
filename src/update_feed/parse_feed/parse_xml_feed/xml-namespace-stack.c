#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

bool
add_namespace_to_stack(struct xml_namespace_stack *stack, const char *name, const char *uri)
{
	INFO("Adding \"%s\" namespace identified as \"%s\" to stack.", uri, name);
	if (stack->top == stack->lim) {
		void *temp = realloc(stack->buf, sizeof(struct xml_namespace) * (stack->lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory to expand namespace stack!");
			return false;
		}
		stack->buf = temp;
		++(stack->lim);
	}
	stack->buf[stack->top].name = crtas(name, strlen(name));
	if (stack->buf[stack->top].name == NULL) {
		return false;
	}
	stack->buf[stack->top].uri = crtas(uri, strlen(uri));
	if (stack->buf[stack->top].uri == NULL) {
		free_string(stack->buf[stack->top].name);
		return false;
	}
	++(stack->top);
	return true;
}

void
pop_namespace_from_stack(struct xml_namespace_stack *stack)
{
	if (stack->top == 0) {
		return;
	}
	INFO("Removing \"%s\" namespace from stack.", stack->buf[stack->top - 1].uri->ptr);
	free_string(stack->buf[stack->top - 1].name);
	free_string(stack->buf[stack->top - 1].uri);
	--(stack->top);
}

const struct string *
find_namespace_uri_by_its_name(const struct xml_namespace_stack *namespaces, const char *name, size_t name_len)
{
	for (size_t i = 0; i < namespaces->top; ++i) {
		if ((name_len == namespaces->buf[i].name->len)
		    && (memcmp(name, namespaces->buf[i].name->ptr, name_len) == 0))
		{
			return namespaces->buf[i].uri;
		}
	}
	WARN("Couldn't find namespace URI for the \"%s\" identifier!", name);
	return NULL;
}

void
free_namespace_stack(struct xml_namespace_stack *stack)
{
	INFO("Freeing namespace stack.");
	for (size_t i = 0; i < stack->top; ++i) {
		free_string(stack->buf[i].name);
		free_string(stack->buf[i].uri);
	}
	free(stack->buf);
}
