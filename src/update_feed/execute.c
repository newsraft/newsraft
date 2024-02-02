#include "update_feed/update_feed.h"

download_status
execute_feed(const struct string *cmd, struct stream_callback_data *data)
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
			enum XML_Status status = XML_Parse(data->xml_parser, content->ptr, content->len, false);
			free_xml_parser(data);
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
			free_json_parser(data);
			if (status != yajl_status_ok) {
				fail_status("JSON parser ran into an error: %s", yajl_status_to_string(status));
				goto error;
			}
			break;
		}
	}
	free_string(real_cmd);
	free_string(content);
	return DOWNLOAD_SUCCEEDED;
error:
	free_string(real_cmd);
	free_string(content);
	return DOWNLOAD_FAILED;
}
