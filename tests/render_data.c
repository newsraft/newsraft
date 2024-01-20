#include <string.h>
#include "newsraft.h"

struct block_sample {
	const char *data;
	render_block_format type;
	bool needs_trimming;
};

struct render_data_test {
	struct block_sample blocks[5];
	size_t blocks_count;
	wchar_t *lines[5];
	size_t lines_count;
};

const struct render_data_test render_tests[] = {
	/*  0 */ {{ {"Hello, world!",   TEXT_PLAIN, 0}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	/*  1 */ {{ {"Hello, world!",   TEXT_PLAIN, 1}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	/*  2 */ {{ {"Hello, world!\n", TEXT_PLAIN, 0}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	/*  3 */ {{ {"Hello, world!\n", TEXT_PLAIN, 1}                             }, 1, {L"Hello, world!",      NULL,     NULL,     NULL    }, 1},
	/*  4 */ {{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 0} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	/*  5 */ {{ {"Hello, world!",   TEXT_PLAIN, 0}, {"Test!",   TEXT_PLAIN, 1} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	/*  6 */ {{ {"Hello, world!",   TEXT_PLAIN, 1}, {"Test!",   TEXT_PLAIN, 0} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	/*  7 */ {{ {"Hello, world!",   TEXT_PLAIN, 1}, {"Test!",   TEXT_PLAIN, 1} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},
	/*  8 */ {{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 0} }, 2, {L"Hello, world!",      L"Test!", NULL,     NULL    }, 2},
	/*  9 */ {{ {"Hello, world!\n", TEXT_PLAIN, 0}, {"Test!\n", TEXT_PLAIN, 1} }, 2, {L"Hello, world!",      L"Test!", NULL,     NULL    }, 2},
	/* 10 */ {{ {"Hello, world!\n", TEXT_PLAIN, 1}, {"Test!\n", TEXT_PLAIN, 0} }, 2, {L"Hello, world!Test!", NULL,     NULL,     NULL    }, 1},

	/* 11 */ {{ {"A\nB\nC\nD",      TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"C",     L"D"    }, 4},
	/* 12 */ {{ {"A\nB\nC\nD\n",    TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"C",     L"D"    }, 4},
	/* 13 */ {{ {"A\n\nC\nD\n",     TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"",      L"C",     L"D"    }, 4},
	/* 14 */ {{ {"A\nB\n\nD\n",     TEXT_PLAIN, 0}                             }, 1, {L"A",                  L"B",     L"",      L"D"    }, 4},
};

int
main(void)
{
	struct render_blocks_list blocks = {0};
	struct render_result result = {0};
	for (size_t i = 0; i < sizeof(render_tests) / sizeof(*render_tests); ++i) {
		memset(&blocks, 0, sizeof(struct render_blocks_list));
		memset(&result, 0, sizeof(struct render_result));
		for (size_t j = 0; j < render_tests[i].blocks_count; ++j) {
			add_render_block(
				&blocks,
				render_tests[i].blocks[j].data,
				strlen(render_tests[i].blocks[j].data),
				render_tests[i].blocks[j].type,
				render_tests[i].blocks[j].needs_trimming
			);
		}
		render_data(&result, &blocks, 80);
		if (result.lines_len != render_tests[i].lines_count) {
			fprintf(stderr, "%s: test %zu fails (invalid number of lines)!\n", __FILE__, i);
			return 1;
		}
		for (size_t k = 0; k < result.lines_len; ++k) {
			if (wcscmp(result.lines[k].ws->ptr, render_tests[i].lines[k]) != 0) {
				fprintf(stderr, "%s: test %zu fails (invalid lines content)!\n", __FILE__, i);
				return 1;
			}
		}
	}
	return 0;
}
