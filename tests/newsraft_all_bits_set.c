#include "newsraft.h"

int
main(void)
{
	uint8_t a1 = 0xFF;
	uint16_t a2 = 0xFFFF;
	uint32_t a4 = 0xFFFFFFFF;
	uint64_t a8 = 0xFFFFFFFFFFFFFFFF;
	if (!NEWSRAFT_ALL_BITS_SET(a1, uint8_t)) return 1;
	if (!NEWSRAFT_ALL_BITS_SET(a2, uint16_t)) return 1;
	if (!NEWSRAFT_ALL_BITS_SET(a4, uint32_t)) return 1;
	if (!NEWSRAFT_ALL_BITS_SET(a8, uint64_t)) return 1;

	uint8_t b1 = 0xFE;
	uint16_t b2 = 0xFFFE;
	uint32_t b4 = 0xFFFFFFFE;
	uint64_t b8 = 0xFFFFFFFFFFFFFFFE;
	if (NEWSRAFT_ALL_BITS_SET(b1, uint8_t)) return 1;
	if (NEWSRAFT_ALL_BITS_SET(b2, uint16_t)) return 1;
	if (NEWSRAFT_ALL_BITS_SET(b4, uint32_t)) return 1;
	if (NEWSRAFT_ALL_BITS_SET(b8, uint64_t)) return 1;

	return 0;
}
