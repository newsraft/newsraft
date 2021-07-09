#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include "feedeater.h"
#include "config.h"

struct {
	const char *name;
	const char *column;
	const int num;
} meta_data[] = {
	{"Feed", "feed", ITEM_COLUMN_FEED},
	{"Title", "title", ITEM_COLUMN_TITLE},
	{"Author", "author", ITEM_COLUMN_AUTHOR},
	{"Category", "category", ITEM_COLUMN_CATEGORY},
	{"Link", "url", ITEM_COLUMN_URL},
	{"Comments", "comments", ITEM_COLUMN_COMMENTS},
};

int
contents_menu(struct string *feed_url, struct item_entry *item_data)
{
	struct string *buf = create_string();
	if (buf == NULL) {
		debug_write(DBG_WARN, "not enough memory for contents buffer\n");
		status_write("[insufficient memory] not enough memory for contents buffer");
		return MENU_CONTENT_ERROR;
	}
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT * FROM items WHERE feed = ? AND guid = ? AND url = ? LIMIT 1", -1, &res, 0);
	if (rc != SQLITE_OK) {
		debug_write(DBG_WARN, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		free_string(&buf);
		sqlite3_finalize(res);
		return MENU_CONTENT_ERROR;
	}
	sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(res, 2, item_data->guid->ptr, item_data->guid->len, NULL);
	sqlite3_bind_text(res, 3, item_data->url->ptr, item_data->url->len, NULL);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "could not find that item\n");
		free_string(&buf);
		sqlite3_finalize(res);
		return MENU_CONTENT_ERROR;
	}
	char *text, *saveptr = NULL;
	size_t text_len;
	char *temp_config_contents_meta_data = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (temp_config_contents_meta_data == NULL) {
		debug_write(DBG_ERR, "not enough memory for tokenizing order of meta data tags\n");
		free_string(&buf);
		sqlite3_finalize(res);
		return MENU_CONTENT_ERROR;
	}
	strcpy(temp_config_contents_meta_data, config_contents_meta_data);
	char *meta_tag = strtok_r(temp_config_contents_meta_data, ",", &saveptr);
	do {
		if (strcmp(meta_tag, "date") == 0) {
			time_t epoch_time = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
			if (epoch_time > 0) {
				struct tm ts = *localtime(&epoch_time);
				char time_str[100];
				if (strftime(time_str, sizeof(time_str), config_contents_date_format, &ts) != 0) {
					cat_string_array(buf, "Date: ", (size_t)6);
					cat_string_array(buf, time_str, strlen(time_str));
					cat_string_char(buf, '\n');
				}
			}
			continue;
		}
		text = NULL;
		for (size_t i = 0; i < LENGTH(meta_data); ++i) {
			if (strcmp(meta_tag, meta_data[i].column) == 0) {
				text = (char *)sqlite3_column_text(res, meta_data[i].num);
				if (text == NULL) break;
				text_len = strlen(text);
				if (text_len != 0) {
					cat_string_array(buf, "<div>", 5);
					cat_string_array(buf, (char *)meta_data[i].name, strlen(meta_data[i].name));
					cat_string_array(buf, ": ", (size_t)2);
					cat_string_array(buf, text, text_len);
					cat_string_array(buf, "</div>\n", 7);
				}
				break;
			}
		}
	} while ((meta_tag = strtok_r(NULL, ",", &saveptr)) != NULL);
	free(temp_config_contents_meta_data);
	text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	if (text != NULL) {
		char *expanded_text = expand_xml_entities(text, strlen(text));
		if (expanded_text == NULL) {
			debug_write(DBG_ERR, "not enough memory for expanding html entities\n");
			free_string(&buf);
			sqlite3_finalize(res);
			return MENU_CONTENT_ERROR;
		}
		cat_string_char(buf, '\n');
		text_len = strlen(expanded_text);
		if (text_len != 0) {
			cat_string_array(buf, expanded_text, text_len);
		}
		free(expanded_text);
	}
	sqlite3_finalize(res);

	char temp_file[1000] = "/tmp/feedeater-XXXXXX";
	char temp_file_full[1000];
	(void)mkstemp(temp_file);
	strcpy(temp_file_full, temp_file);
	strcat(temp_file_full, ".html");
	rename(temp_file, temp_file_full);
	FILE* tempf = fopen(temp_file_full, "w");
	if (tempf == NULL) {
		return MENU_CONTENT_ERROR;
	}

	fwrite(buf->ptr, sizeof(char), buf->len, tempf);
	fclose(tempf);
	free_string(&buf);
	hide_items();

	savetty();
	endwin();
	pid_t process = fork();
	if (process == 0) {
		// child
		/*input_delete();*/
		/*status_delete();*/
		free_items();
		free_sets();
		db_stop();
		free_data_dir_path();
		free_conf_dir_path();
		debug_stop();
		execl("/bin/w3m", "w3m", "-T", "text/html", temp_file_full, NULL);
		exit(EXIT_FAILURE);
	} else if (process > 0) {
		// parent
		wait(NULL);
		db_update_item_int(feed_url, item_data, "unread", 0);
	}
	resetty();

	remove(temp_file_full);

	return MENU_ITEMS;
}
