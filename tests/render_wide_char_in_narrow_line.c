#include <string.h>
#include "newsraft.h"

// A double-width character must not hang or crash the line processor when
// indentation leaves only one free column on the line (width - indent == 1).

bool
newsraft_test(void)
{
	const char *samples[] = {
		"<ul><li>first release 🎉</li></ul>",                  // indent 8 at width 9
		"<blockquote><p>漢字 quoted</p></blockquote>",         // indent 4 at width 5
		"<ul><li>a<ul><li>nested 🎉 item</li></ul></li></ul>", // indent 16 at width 17
	};
	for (size_t i = 0; i < LENGTH(samples); ++i) {
		for (size_t width = 1; width <= 20; ++width) {
			struct render_blocks_list blocks = {0};
			struct render_result result = {0};
			add_render_block(&blocks, samples[i], strlen(samples[i]), TEXT_HTML, true);
			render_data(NULL, &result, &blocks, width);
			if (result.lines_len == 0) {
				fprintf(stderr, "%s: sample %zu at width %zu produced nothing!\n", __FILE__, i, width);
				return false;
			}
			free_render_result(&result);
			free_render_blocks(&blocks);
		}
	}
	return true;
}
