#include <string.h>
#include "newsraft.h"

bool
newsraft_test(void)
{
	NEWSRAFT_ASSERT(freopen("tests/assets/convert_opml_to_feeds.xml", "r", stdin));
	NEWSRAFT_ASSERT(freopen("newsraft-test-feeds", "w", stdout));

	convert_opml_to_feeds();

	FILE *example_stream = fopen("tests/assets/convert_opml_to_feeds.txt", "r");
	FILE *generated_stream = fopen("newsraft-test-feeds", "r");
	NEWSRAFT_ASSERT(example_stream);
	NEWSRAFT_ASSERT(generated_stream);

	char example[10000];
	char generated[10000];
	size_t example_len = fread(example, 1, sizeof(example), example_stream);
	size_t generated_len = fread(generated, 1, sizeof(generated), generated_stream);
	example[example_len] = '\0';
	generated[generated_len] = '\0';
	fclose(example_stream);
	fclose(generated_stream);

	if (strcmp(example, generated) != 0) {
		fprintf(stderr, "Generated feeds file:\n%s\n", generated);
		return false;
	}

	return true;
}
