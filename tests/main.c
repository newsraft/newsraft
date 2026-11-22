#include <locale.h>

#include "newsraft.h"

bool newsraft_test(void);

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	setlocale(LC_ALL, "");
	set_db_path(":memory:");
	set_cfg_string(NULL, CFG_NOTIFICATION_COMMAND, "", 0);

	return newsraft_test() ? 0 : 1;
}
