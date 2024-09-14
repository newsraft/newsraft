#include <signal.h>
#include "newsraft.h"

static void
execute_feed(const struct string *cmd, struct feed_update_state *data)
{
	struct string *real_cmd = crtas(cmd->ptr + 2, cmd->len - 3);
	struct string *content = crtes(10000);
	FILE *p = popen(real_cmd->ptr, "r");
	if (p == NULL) {
		goto error;
	}
	for (int c = fgetc(p); c != EOF; c = fgetc(p)) {
		catcs(content, c);
	}
	pclose(p);
	for (const char *i = content->ptr; *i != '\0'; ++i) {
		if (*i == '<') {
			INFO("The output has \"<\" character in the beginning - engaging XML parser.");
			if (setup_xml_parser(data) == false) {
				FAIL("Failed to setup XML parser!");
				goto error;
			}
			enum XML_Status status = XML_Parse(data->xml_parser, content->ptr, content->len, XML_FALSE);
			if (status == XML_STATUS_OK) {
				status = XML_Parse(data->xml_parser, NULL, 0, XML_TRUE); // Final parsing call
			}
			if (status != XML_STATUS_OK) {
				fail_status("XML parser ran into an error: %s", XML_ErrorString(XML_GetErrorCode(data->xml_parser)));
				goto error;
			}
			break;
		} else if (*i == '{') {
			INFO("The output has \"{\" character in the beginning - engaging JSON parser.");
			if (setup_json_parser(data) == false) {
				FAIL("Failed to setup JSON parser!");
				goto error;
			}
			yajl_status status = yajl_parse(data->json_parser, (const unsigned char *)content->ptr, content->len);
			if (status == yajl_status_ok) {
				status = yajl_complete_parse(data->json_parser); // Final parsing call
			}
			if (status != yajl_status_ok) {
				fail_status("JSON parser ran into an error: %s", yajl_status_to_string(status));
				goto error;
			}
			break;
		}
	}
	free_string(real_cmd);
	free_string(content);
	return;
error:
	free_string(real_cmd);
	free_string(content);
	data->is_failed = true;
	return;
}

static bool
engage_with_not_executed_feed(struct feed_update_state *data)
{
	if (data->is_finished == false
		&& data->is_in_progress == false
		&& data->feed_entry->link->ptr[0] == '$')
	{
		data->is_in_progress = true;
		return true;
	}
	return false;
}

void *
executor_worker(void *dummy)
{
	(void)dummy;
	while (they_want_us_to_stop == false) {
		struct feed_update_state *target = queue_pull(&engage_with_not_executed_feed);
		if (target == NULL) {
			threads_take_a_nap(NEWSRAFT_THREAD_SHRUNNER);
			continue;
		}
		execute_feed(target->feed_entry->link, target);
		target->is_downloaded = true;
		threads_wake_up(NEWSRAFT_THREAD_DBWRITER);
	}
	return NULL;
}
