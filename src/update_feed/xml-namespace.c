#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

struct {
	const char *name;
	XML_StartElementHandler parse_element_beginning;
	XML_EndElementHandler parse_element_end;
} namespace_handlers[] = {
	{"http://www.w3.org/2005/Atom", &parse_atom10_element_beginning, &parse_atom10_element_end},
	{"http://purl.org/rss/1.0/", &parse_rss10_element_beginning, &parse_rss10_element_end},
	{"http://purl.org/dc/elements/1.1/", &parse_dc_element_beginning, &parse_dc_element_end},
	{"http://purl.org/atom/ns#", &parse_atom03_element_beginning, &parse_atom03_element_end},
	{"http://channel.netscape.com/rdf/simple/0.9/", &parse_rss10_element_beginning, &parse_rss10_element_end},
	{"http://purl.org/net/rss1.1#", &parse_rss10_element_beginning, &parse_rss10_element_end},
};

static char *
get_namespace_name(const XML_Char *name)
{
	char *separator_pos = strrchr(name, NAMESPACE_SEPARATOR);
	if (separator_pos == NULL) {
		return NULL;
	}
	size_t namespace_len = separator_pos - name;
	if (namespace_len > 1000U) {
		debug_write(DBG_FAIL, "Got enormous long namespace name!\n");
		return NULL;
	}
	char *namespace = malloc(sizeof(char) * (namespace_len + 1));
	if (namespace == NULL) {
		debug_write(DBG_FAIL, "Not enough memory for a name of namespace!\n");
		return NULL;
	}
	memcpy(namespace, name, sizeof(char) * namespace_len);
	namespace[namespace_len] = '\0';
	return namespace;
}

static char *
get_tag_name(const XML_Char *name)
{
	size_t tag_name_len;
	char *separator_pos = strrchr(name, NAMESPACE_SEPARATOR);
	if (separator_pos == NULL) {
		tag_name_len = strlen(name);
	} else {
		tag_name_len = name + strlen(name) - separator_pos - 1;
	}
	if (tag_name_len > 1000U) {
		debug_write(DBG_FAIL, "Got enormous long tag name!\n");
		return NULL;
	}
	char *tag_name = malloc(sizeof(char) * (tag_name_len + 1));
	if (tag_name == NULL) {
		debug_write(DBG_FAIL, "Not enough memory for a name of tag!\n");
		return NULL;
	}
	if (separator_pos == NULL) {
		memcpy(tag_name, name, sizeof(char) * tag_name_len);
	} else {
		memcpy(tag_name, separator_pos + 1, sizeof(char) * tag_name_len);
	}
	tag_name[tag_name_len] = '\0';
	return tag_name;
}

int
parse_namespace_element_beginning(void *userData, const XML_Char *name, const XML_Char **atts) {
	char *namespace = get_namespace_name(name);
	if (namespace == NULL) {
		return 1; // failure
	}
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			char *tag_name = get_tag_name(name);
			if (tag_name == NULL) {
				free(namespace);
				return 1; // failure
			}
			namespace_handlers[i].parse_element_beginning(userData, tag_name, atts);
			free(tag_name);
			free(namespace);
			return 0; // success
		}
	}
	free(namespace);
	return 1; // failure
}

int
parse_namespace_element_end(void *userData, const XML_Char *name) {
	char *namespace = get_namespace_name(name);
	if (namespace == NULL) {
		return 1; // failure
	}
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].name) == 0) {
			char *tag_name = get_tag_name(name);
			if (tag_name == NULL) {
				free(namespace);
				return 1; // failure
			}
			namespace_handlers[i].parse_element_end(userData, tag_name);
			free(tag_name);
			free(namespace);
			return 0; // success
		}
	}
	free(namespace);
	return 1; // failure
}
