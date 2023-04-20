#include <signal.h>
#include "newsraft.h"

bool
register_signal_handlers(void)
{
	struct sigaction act = {0};
	act.sa_handler = &tell_program_to_terminate_safely_and_quickly;
	if (sigaction(SIGQUIT, &act, NULL) == 0) {
		if (sigaction(SIGINT, &act, NULL) == 0) {
			if (sigaction(SIGTERM, &act, NULL) == 0) {
				return true;
			}
		}
	}
	fputs("Failed to register signal handlers!\n", stderr);
	return false;
}
