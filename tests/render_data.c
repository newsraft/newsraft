#include <string.h>
#include "newsraft.h"

struct block_sample {
	const char *data;
	render_block_format type;
	size_t separators;
};

struct render_data_test {
	struct block_sample blocks[5];
	size_t blocks_count;
	wchar_t *lines[5];
	size_t lines_count;
};

const struct render_data_test render_tests[] = {
	{{ {"Hello, world!",   TEXT_PLAIN, 0}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!",   TEXT_PLAIN, 1}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!",   TEXT_PLAIN, 2}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!",   TEXT_PLAIN, 3}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},

	{{ {"Hello, world!\n", TEXT_PLAIN, 0}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!\n", TEXT_PLAIN, 1}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!\n", TEXT_PLAIN, 2}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!\n", TEXT_PLAIN, 3}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},

	{{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 0} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 1} }, 2, {L"Hello, world!",      L"Test!", NULL,     NULL    }, 2},
	{{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 2} }, 2, {L"Hello, world!",      L"",      L"Test!", NULL    }, 3},
	{{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 3} }, 2, {L"Hello, world!",      L"",      L"",      L"Test!"}, 4},

	{{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 0} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	{{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 1} }, 2, {L"Hello, world!",      L"Test!", NULL,     NULL    }, 2},
	{{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 2} }, 2, {L"Hello, world!",      L"",      L"Test!", NULL    }, 3},
	{{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 3} }, 2, {L"Hello, world!",      L"",      L"",      L"Test!"}, 4},

	{{ {"A\nB\nC\nD",      TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"C",     L"D"    }, 4},
	{{ {"A\nB\nC\nD\n",    TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"C",     L"D"    }, 4},
	{{ {"A\n\nC\nD\n",     TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"",      L"C",     L"D"    }, 4},
	{{ {"A\nB\n\nD\n",     TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"",      L"D"    }, 4},
};

int
main(void)
{
	struct render_blocks_list blocks = {0};
	struct render_result result = {0};
	list_menu_width = 80;
	for (size_t i = 0; i < sizeof(render_tests) / sizeof(*render_tests); ++i) {
		memset(&blocks, 0, sizeof(struct render_blocks_list));
		memset(&result, 0, sizeof(struct render_result));
		for (size_t j = 0; j < render_tests[i].blocks_count; ++j) {
			join_render_block(
				&blocks,
				render_tests[i].blocks[j].data,
				strlen(render_tests[i].blocks[j].data),
				render_tests[i].blocks[j].type,
				render_tests[i].blocks[j].separators
			);
		}
		render_data(&result, &blocks);
		if (result.lines_len != render_tests[i].lines_count) {
			return 1;
		}
		for (size_t k = 0; k < result.lines_len; ++k) {
			if (wcscmp(result.lines[k].ws->ptr, render_tests[i].lines[k]) != 0) {
				return 1;
			}
		}
	}
	return 0;
}
