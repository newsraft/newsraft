#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "feedeater.h"

struct {
	const char *name;
	XML_StartElementHandler start_element_handler;
	XML_EndElementHandler end_element_handler;
} namespace_handlers[] = {
	{"http://www.w3.org/2005/Atom", &atom_10_start, &atom_10_end},
};

static char *
get_namespace(const XML_Char *name)
{
	char *separator_pos = strrchr(name, ':');
	if (separator_pos == NULL) return NULL;
	char *namespace = malloc(sizeof(char) * (separator_pos - name + 1));
	if (namespace == NULL) { fprintf(stderr, "memory allocation failed!\n"); return NULL; }
	memcpy(namespace, name, sizeof(char) * (separator_pos - name));
	namespace[separator_pos - name] = '\0';
	return namespace;
}

static char *
get_tag_name(const XML_Char *name)
{
	char *separator_pos = strrchr(name, ':');
	if (separator_pos == NULL) return NULL;
	size_t tag_name_len = name + strlen(name) - separator_pos;
	char *tag_name = malloc(sizeof(char) * tag_name_len);
	if (tag_name == NULL) { fprintf(stderr, "memory allocation failed!\n"); return NULL; }
	memcpy(tag_name, separator_pos + 1, sizeof(char) * tag_name_len);
	return tag_name;
}

int
process_namespaced_tag_start(void *userData, const XML_Char *name, const XML_Char **atts) {
	char *namespace = get_namespace(name);
	if (namespace == NULL) return 1;
	char *tag_name = get_tag_name(name);
	for (int i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			namespace_handlers[i].start_element_handler(userData, tag_name, atts);
		}
	}
	free(tag_name);
	free(namespace);
	return 0;
}

int
process_namespaced_tag_end(void *userData, const XML_Char *name) {
	char *namespace = get_namespace(name);
	if (namespace == NULL) return 1;
	char *tag_name = get_tag_name(name);
	for (int i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			namespace_handlers[i].end_element_handler(userData, tag_name);
		}
	}
	free(tag_name);
	free(namespace);
	return 0;
}
