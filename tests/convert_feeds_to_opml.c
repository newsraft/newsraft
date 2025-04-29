#include <locale.h>
#include <string.h>
#include "newsraft.h"

int
main(void)
{
	setlocale(LC_ALL, "");
	set_db_path("./newsraft-test-database");
	set_feeds_path("./tests/assets/convert_opml_to_feeds.txt");
	db_init();
	parse_feeds_file();
	db_stop();

	if (freopen("newsraft-test-feeds", "w", stdout) == NULL) {
		return 1;
	}

	convert_feeds_to_opml();

	FILE *example_stream = fopen("tests/assets/convert_feeds_to_opml.xml", "r");
	FILE *generated_stream = fopen("newsraft-test-feeds", "r");
	if (example_stream == NULL || generated_stream == NULL) {
		return 1;
	}
	char example[10000];
	char generated[10000];
	size_t example_len = fread(example, 1, sizeof(example), example_stream);
	size_t generated_len = fread(generated, 1, sizeof(generated), generated_stream);
	example[example_len] = '\0';
	generated[generated_len] = '\0';
	fclose(example_stream);
	fclose(generated_stream);

	if (strcmp(example, generated) != 0) {
		fprintf(stderr, "Generated OPML file:\n%s\n", generated);
		return 1;
	}

	return 0;
}
