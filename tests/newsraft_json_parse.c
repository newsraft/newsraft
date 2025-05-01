#include <string.h>
#include "newsraft.h"

int
test1(const char *content, size_t size)
{
	struct feed_update_state data = {};
	setup_json_parser(&data);
	if (!newsraft_json_parse(&data, content, size)) {
		return 1;
	}

	if (strcmp(data.feed.title->ptr, "My Example Feed") != 0) return 1;
	if (strcmp(data.feed.url->ptr, "https://example.org/") != 0) return 1;

	size_t items_count = 0;
	for (struct getfeed_item *i = data.feed.item; i != NULL; i = i->next) {
		items_count += 1;
	}
	if (items_count != 2) return 1;

	struct getfeed_item *item1 = data.feed.item;
	struct getfeed_item *item2 = data.feed.item->next;

	if (strcmp(item1->guid->ptr, "1") != 0) return 1;
	if (strcmp(item2->guid->ptr, "2") != 0) return 1;
	if (strcmp(item1->url->ptr, "https://example.org/initial-post") != 0) return 1;
	if (strcmp(item2->url->ptr, "https://example.org/second-item")  != 0) return 1;
	if (strcmp(item1->content->ptr, "\x1F^\x1Ftype=text/html\x1Ftext=<p>Hello, world!</p>") != 0) return 1;
	if (strcmp(item2->content->ptr, "\x1F^\x1Ftext=This is a second item.") != 0) return 1;

	return 0;
}

int
test2(const char *content, size_t size)
{
	struct feed_update_state data = {};
	setup_json_parser(&data);
	if (!newsraft_json_parse(&data, content, size)) {
		return 1;
	}

	if (strcmp(data.feed.title->ptr, "The Record") != 0) return 1;
	if (strcmp(data.feed.url->ptr, "http://therecord.co/") != 0) return 1;

	size_t items_count = 0;
	for (struct getfeed_item *i = data.feed.item; i != NULL; i = i->next) {
		items_count += 1;
	}
	if (items_count != 1) return 1;

	struct getfeed_item *item = data.feed.item;

	if (strcmp(item->guid->ptr, "http://therecord.co/chris-parrish") != 0) return 1;
	if (strcmp(item->title->ptr, "Special #1 - Chris Parrish") != 0) return 1;
	if (strcmp(item->url->ptr, "http://therecord.co/chris-parrish") != 0) return 1;
	if (item->publication_date != 1399669440LL) return 1;

	size_t texts_count = 0;
	for (const char *i = item->content->ptr; true; ++texts_count) {
		i = strstr(i + 1, "\x1Ftext=");
		if (i == NULL) {
			break;
		}
	}
	if (texts_count != 3) return 1;

	if (strstr(item->content->ptr, "type=text/html") == NULL) return 1;

	if (strstr(item->attachments->ptr, "url=http://therecord.co/downloads/The-Record-sp1e1-ChrisParrish.m4a") == NULL) return 1;
	if (strstr(item->attachments->ptr, "type=audio/x-m4a") == NULL) return 1;
	if (strstr(item->attachments->ptr, "size=89970236") == NULL) return 1;
	if (strstr(item->attachments->ptr, "duration=6629") == NULL) return 1;

	return 0;
}

int
test3(const char *content, size_t size)
{
	struct feed_update_state data = {};
	setup_json_parser(&data);
	if (!newsraft_json_parse(&data, content, size)) {
		return 1;
	}

	if (strcmp(data.feed.title->ptr, "Brent Simmons’s Microblog") != 0) return 1;
	if (strcmp(data.feed.url->ptr, "https://example.org/") != 0) return 1;
	if (strcmp(data.feed.persons->ptr, "\x1F^\x1Ftype=author\x1Fname=Brent Simmons\x1Furl=http://example.org/") != 0) return 1;

	size_t items_count = 0;
	for (struct getfeed_item *i = data.feed.item; i != NULL; i = i->next) {
		items_count += 1;
	}
	if (items_count != 1) return 1;

	struct getfeed_item *item = data.feed.item;

	if (strcmp(item->guid->ptr, "2347259") != 0) return 1;
	if (strcmp(item->url->ptr, "https://example.org/2347259") != 0) return 1;
	if (strcmp(item->content->ptr, "\x1F^\x1Ftext=Cats are neat. \n\nhttps://example.org/cats") != 0) return 1;
	if (item->publication_date != 1455052920LL) return 1;

	return 0;
}

int
main(void)
{
	const char *files[] = {
		"tests/assets/newsraft_json_parse_1.json",
		"tests/assets/newsraft_json_parse_2.json",
		"tests/assets/newsraft_json_parse_3.json",
	};
	char jsons[3][2000] = {};
	size_t lens[3] = {};
	for (size_t i = 0; i < 3; ++i) {
		FILE *f = fopen(files[i], "r");
		if (f == NULL) {
			return 1;
		}
		lens[i] = fread(jsons[i], 1, sizeof(jsons[i]), f);
		jsons[i][lens[i]] = '\0';
		fclose(f);
	}

	if (test1(jsons[0], lens[0])) return 1;
	if (test2(jsons[1], lens[1])) return 1;
	if (test3(jsons[2], lens[2])) return 1;

	return 0;
}
