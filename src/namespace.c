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
	if (separator_pos == NULL) {
		return NULL;
	}
	size_t namespace_len = separator_pos - name;
	char *namespace = malloc(sizeof(char) * (namespace_len + 1));
	if (namespace == NULL) {
		debug_write(DBG_ERR, "not enough memory for namespace name\n");
		return NULL;
	}
	memcpy(namespace, name, sizeof(char) * namespace_len);
	namespace[namespace_len] = '\0';
	return namespace;
}

static char *
get_tag_name(const XML_Char *name)
{
	char *separator_pos = strrchr(name, ':');
	if (separator_pos == NULL) {
		return NULL;
	}
	size_t tag_name_len = name + strlen(name) - separator_pos - 1;
	char *tag_name = malloc(sizeof(char) * (tag_name_len + 1));
	if (tag_name == NULL) {
		debug_write(DBG_ERR, "not enough memory for tag name\n");
		return NULL;
	}
	memcpy(tag_name, separator_pos + 1, sizeof(char) * tag_name_len);
	tag_name[tag_name_len] = '\0';
	return tag_name;
}

int
process_namespaced_tag_start(void *userData, const XML_Char *name, const XML_Char **atts) {
	char *namespace = get_namespace(name);
	if (namespace == NULL) {
		return 1;
	}
	char *tag_name = get_tag_name(name);
	if (tag_name == NULL) {
		return 1;
	}
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			namespace_handlers[i].start_element_handler(userData, tag_name, atts);
			break;
		}
	}
	free(tag_name);
	free(namespace);
	return 0;
}

int
process_namespaced_tag_end(void *userData, const XML_Char *name) {
	char *namespace = get_namespace(name);
	if (namespace == NULL) {
		return 1;
	}
	char *tag_name = get_tag_name(name);
	if (tag_name == NULL) {
		return 1;
	}
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			namespace_handlers[i].end_element_handler(userData, tag_name);
			break;
		}
	}
	free(tag_name);
	free(namespace);
	return 0;
}
