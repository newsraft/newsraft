#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

bool
prepend_default_namespace(struct xml_default_namespace **first_def_ns, const char *uri_to_prepend, size_t uri_len)
{
	INFO("Setting default namespace to \"%s\".", uri_to_prepend);
	struct xml_default_namespace *new_namespace = malloc(sizeof(struct xml_default_namespace));
	if (new_namespace == NULL) {
		return false;
	}
	new_namespace->uri = crtas(uri_to_prepend, uri_len);
	if (new_namespace->uri == NULL) {
		free(new_namespace);
		return false;
	}
	new_namespace->next = *first_def_ns;
	*first_def_ns = new_namespace;
	return true;
}

void
discard_default_namespace(struct xml_default_namespace **first_def_ns)
{
	if (*first_def_ns == NULL) {
		return;
	}
	INFO("Discarding previous default namespace.");
	struct xml_default_namespace *namespace_to_free = *first_def_ns;
	*first_def_ns = (*first_def_ns)->next;
	free_string(namespace_to_free->uri);
	free(namespace_to_free);
}

void
free_default_namespaces(struct xml_default_namespace *first_def_ns)
{
	INFO("Freeing default namespaces.");
	struct xml_default_namespace *ns = first_def_ns;
	struct xml_default_namespace *temp;
	while (ns != NULL) {
		temp = ns;
		ns = ns->next;
		free_string(temp->uri);
		free(temp);
	}
}
